# Requirements:
# pip install pvporcupine

import os
import ssl
import time
import json
import base64
import struct
import dotenv
import logging
import pyaudio
import asyncio
import threading
import websockets
import pvporcupine

# Voice
VOICE = "ash"
VOICE2 = "alloy"
MODEL = "gpt-4o-realtime-preview-2024-10-01" 
#MODEL = "gpt-4o-mini-realtime-preview-2024-12-17"

# Constants
INSTRUCTIONS = f"""
You are Rob, a kind and friendly home assistant.
"""

# Wake word
KEYWORDS = ['hey siri']

# Keys
dotenv.load_dotenv("keys.sh")
OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY")

# Picovoice key is from https://console.picovoice.ai
PICOVOICE_KEY = os.environ.get("PICOVOICE_KEY")
if PICOVOICE_KEY is None:
    print("Check that your keys.sh file has export PICOVOICE_KEY=key in it")
    exit()

# Logger setup
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# Audio handler
class AudioHandler:
    def __init__(self):
        self.p = pyaudio.PyAudio()
        self.stream = None
        self.audio_buffer = b''
        self.chunk_size = 512
        self.format = pyaudio.paInt16
        self.channels = 1
        self.rate = 24000
        self.is_recording = False

    def start_audio_stream(self):
        self.stream = self.p.open(format=self.format, channels=self.channels, rate=self.rate, input=True, frames_per_buffer=self.chunk_size)

    def stop_audio_stream(self):
        if self.stream:
            self.stream.stop_stream()
            self.stream.close()

    def cleanup(self):
        if self.stream:
            self.stop_audio_stream()
        self.p.terminate()

    def start_recording(self):
        self.is_recording = True
        self.audio_buffer = b''
        self.start_audio_stream()

    def stop_recording(self):
        self.is_recording = False
        self.stop_audio_stream()
        return self.audio_buffer

    def record_chunk(self):
        if self.stream and self.is_recording:
            data = self.stream.read(self.chunk_size)
            self.audio_buffer += data
            return data
        return None
    
    def play_audio(self, audio_data):
        def play():
            stream = self.p.open(format=self.format, channels=self.channels, rate=self.rate, output=True)
            stream.write(audio_data)
            stream.stop_stream()
            stream.close()

        playback_thread = threading.Thread(target=play)
        playback_thread.start()

# Realtime client for OpenAI API
class RealtimeClient:
    def __init__(self, instructions, voice=VOICE2):
        self.url = "wss://api.openai.com/v1/realtime"
        self.model = MODEL
        self.api_key = OPENAI_API_KEY
        self.ws = None
        self.audio_handler = AudioHandler()
        self.ssl_context = ssl.create_default_context()
        self.ssl_context.check_hostname = False
        self.ssl_context.verify_mode = ssl.CERT_NONE
        self.instructions = instructions
        self.voice = voice
        self.VAD_turn_detection = True
        self.VAD_config = {
            "type": "server_vad",
            "threshold": 0.5,
            "prefix_padding_ms": 300,
            "silence_duration_ms": 600
        }
        self.session_config = {
            "modalities": ["audio", "text"],
            "instructions": self.instructions,
            "voice": self.voice,
            "input_audio_format": "pcm16",
            "output_audio_format": "pcm16",
            "turn_detection": self.VAD_config if self.VAD_turn_detection else None,
            "input_audio_transcription": {"model": "whisper-1"},
            "temperature": 0.6
        }

    async def connect(self):
        headers = {
            "Authorization": f"Bearer {self.api_key}",
            "OpenAI-Beta": "realtime=v1"
        }
        self.ws = await websockets.connect(f"{self.url}?model={self.model}", additional_headers=headers, ssl=self.ssl_context) # or extra_headers= in pre 14.0 websockets, see https://websockets.readthedocs.io/en/latest/howto/upgrade.html#extra-headers-additional-headers and https://websockets.readthedocs.io/en/stable/project/changelog.html#id7
        logger.info("Successfully connected to OpenAI Realtime API")
        await self.send_event({"type": "session.update", "session": self.session_config})
        #await self.send_event({"type": "response.create"})
        #logger.debug("Sent response.create to initiate conversation")

    async def send_event(self, event):
        await self.ws.send(json.dumps(event))

    async def receive_events(self):
        try:
            async for message in self.ws:
                event = json.loads(message)
                event_type = event.get("type")
                logger.debug(f"Received event type: {event_type}")
                await self.handle_event(event)
        except websockets.ConnectionClosed as e:
            logger.error(f"WebSocket connection closed: {e}")
        except Exception as e:
            logger.error(f"Unexpected error: {e}")

    async def handle_event(self, event):
        event_type = event.get("type")
        if event_type == "response.text.delta":
            print(event["delta"], end="", flush=True)
        elif event_type == "response.audio.delta":
            audio_data = base64.b64decode(event["delta"])
            self.audio_handler.play_audio(audio_data)
        elif event_type == "response.done":
            logger.debug("Response generation completed")
        elif event_type == "session.created":
            logger.debug("Session created")
        else:
            logger.debug(f"Unhandled event type: {event_type}")

    async def send_audio(self):
        logger.debug("Sending audio")
        self.audio_handler.start_recording()
        try:
            while True:
                chunk = self.audio_handler.record_chunk()
                if chunk:
                    base64_chunk = base64.b64encode(chunk).decode('utf-8')
                    await self.send_event({"type": "input_audio_buffer.append", "audio": base64_chunk})
                    await asyncio.sleep(0.01)
                else:
                    break
        except Exception as e:
            logger.error(f"Error during audio recording: {e}")
        finally:
            self.audio_handler.stop_recording()

    async def run(self):
        await self.connect()
        receive_task = asyncio.create_task(self.receive_events())
        await self.send_audio()
        receive_task.cancel()

    async def cleanup(self):
        self.audio_handler.cleanup()
        if self.ws:
            await self.ws.close()

# Wakeword detection using Porcupine
class PorcupineWakeword:
    def __init__(self, keyword_path=None, keywords=KEYWORDS, sensitivity=0.5, callback=None):
        self.callback = callback
        self.porcupine = pvporcupine.create(access_key=PICOVOICE_KEY, keywords=keywords, sensitivities=[sensitivity])
        self.pyaudio_instance = pyaudio.PyAudio()
        self.stream = self.pyaudio_instance.open(format=pyaudio.paInt16, channels=1, rate=self.porcupine.sample_rate, input=True, frames_per_buffer=self.porcupine.frame_length)

    async def start_listening(self):
        print("Listening for wake word...")
        self.is_listening = True
        try:
            while self.is_listening:
                # Read audio data from stream
                audio_data = self.stream.read(self.porcupine.frame_length)
                audio_data = struct.unpack_from("h" * self.porcupine.frame_length, audio_data)
                result = self.porcupine.process(audio_data)
                if result >= 0:  # Wake word detected
                    logging.info("Wake word detected!")
                    self.stop_listening()
                    if self.callback:
                        await self.callback()
        except Exception as e:
            logger.error(f"Error during wake word detection: {e}")

    def stop_listening(self):
        self.is_listening = False
        self.stream.stop_stream()
        self.stream.close()
        self.pyaudio_instance.terminate()

# Voice assistant with wakeword detection
class VoiceAssistant:
    def __init__(self, instructions, voice=VOICE):
        self.realtime_client = RealtimeClient(instructions, voice)
        self.wakeword_detector = PorcupineWakeword(callback=self.start_conversation)
        self.conversation_active = False

    async def start_conversation(self):
        logger.info("Starting conversation...")

        # Connect to OpenAI
        await self.realtime_client.connect()
        asyncio.create_task(self.realtime_client.receive_events())
        self.conversation_active = True
        try:
            while self.conversation_active:
                # Get chunk of audio
                audio_chunk = self.realtime_client.audio_handler.record_chunk()
                if audio_chunk:
                    # Send to OpenAI realtime client
                    base64_chunk = base64.b64encode(audio_chunk).decode('utf-8')
                    await self.realtime_client.send_event({
                        "type": "input_audio_buffer.append",
                        "audio": base64_chunk
                    })

                    # Sleep
                    await asyncio.sleep(0.1)
        except Exception as e:
            logger.error(f"Error during conversation: {e}")
        finally:
            await self.end_conversation()

    async def end_conversation(self):
        logger.info("Ending conversation...")
        self.conversation_active = False
        await self.realtime_client.cleanup()

    async def run(self):
        try:
            await self.wakeword_detector.start_listening()
        except:
            # Handle the case where self.wakeword_detector is None
            print("Wakeword detector not initialized.")

# Run
if __name__ == "__main__":
    assistant = VoiceAssistant(INSTRUCTIONS)
    asyncio.run(assistant.run())
