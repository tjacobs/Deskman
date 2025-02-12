# Voice conversations for the Deskman robot.
# Using OpenAI realtime API and Picovoice porcupine for wake word detection.

# Imports
import os
import ssl
import time
import json
import queue
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

# Prompt
INSTRUCTIONS = f"""
You are Deskman, a friendly home assistance robot, with a physical appearance of a robot head and shoulders on a desk.
"""

# Wake word
KEYWORDS = ['computer']

# Keys
dotenv.load_dotenv("keys.sh")
OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY")

# Picovoice key is from https://console.picovoice.ai
PICOVOICE_KEY = os.environ.get("PICOVOICE_KEY")
if PICOVOICE_KEY is None:
    print("Check that your keys.sh file has export PICOVOICE_KEY=key in it")
    exit()

# Logger setup
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# Audio handler
class AudioHandler:
    def __init__(self):
        self.p = pyaudio.PyAudio()
        self.stream_in = None
        self.stream_out = None
        self.audio_buffer = b''
        self.chunk_size = 512 * 2
        self.format = pyaudio.paInt16
        self.channels = 1
        self.rate = 24000
        self.is_recording = False

        # For output buffering:
        self.play_queue = queue.Queue()
        self.playback_thread = None
        self.playback_running = False

    def start_audio_stream(self):
        self.stream_in = self.p.open(
            format=self.format,
            channels=self.channels,
            rate=self.rate,
            input=True,
            frames_per_buffer=self.chunk_size
        )

    def stop_audio_stream(self):
        if self.stream_in:
            self.stream_in.stop_stream()
            self.stream_in.close()
            self.stream_in = None

    def cleanup(self):
        self.stop_audio_stream()
        self.stop_playback()
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
        if self.stream_in and self.is_recording:
            data = self.stream_in.read(self.chunk_size)
            self.audio_buffer += data
            return data
        return None

    # -- Continuous Playback Logic Below --
    def start_playback(self):
        """Open one output stream and start a playback thread reading from the queue."""
        if self.stream_out is None:
            self.stream_out = self.p.open(
                format=self.format,
                channels=self.channels,
                rate=self.rate,
                output=True
            )
        if not self.playback_running:
            self.playback_running = True
            self.playback_thread = threading.Thread(target=self._play_loop, daemon=True)
            self.playback_thread.start()

    def stop_playback(self):
        """Signal the playback thread to exit and close the stream."""
        self.playback_running = False
        # Send a sentinel (None) to unblock queue.get()
        self.play_queue.put(None)

        # Wait for the thread to finish
        if self.playback_thread is not None:
            self.playback_thread.join()
            self.playback_thread = None

        # Close the output stream
        if self.stream_out is not None:
            self.stream_out.stop_stream()
            self.stream_out.close()
            self.stream_out = None

    def _play_loop(self):
        """Continuously read from play_queue and write to the single output stream."""
        while self.playback_running:
            audio_data = self.play_queue.get()
            if audio_data is None:
                break
            self.stream_out.write(audio_data)
        # End of playback loop

    def queue_audio(self, audio_data: bytes):
        """Public method to enqueue new audio data for playback."""
        if not self.playback_running:
            self.start_playback()
        self.play_queue.put(audio_data)

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
        logger.debug("Successfully connected to OpenAI Realtime API")
        await self.send_event({"type": "session.update", "session": self.session_config})

        # Test asking for a response
        if False:
            await self.send_event({"type": "response.create"})
            logger.debug("Sent response.create to initiate conversation")

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
        if event_type == "response.audio_transcript.delta":
            print(event["delta"], end="", flush=True)
        elif event_type == "response.audio.done":
            print("")            
        elif event_type == "response.audio.delta":
            audio_data = base64.b64decode(event["delta"])
            logger.debug("Got audio: " + str(len(audio_data)))
            self.audio_handler.queue_audio(audio_data)
        elif event_type == "response.done":
            logger.debug("Response generation completed")
        elif event_type == "session.created":
            logger.debug("Session created")
            self.audio_handler.start_playback()
        elif event_type == "error":
            logger.info(event)
        elif event_type == "conversation.item.input_audio_transcription.completed":
            logger.debug(f"Event: {event_type}")
        else:
            logger.info(f"Event: {event_type}")

    async def run(self):
        await self.connect()
        asyncio.create_task(self.receive_events())

    async def cleanup(self):
        self.audio_handler.cleanup()
        if self.ws:
            await self.ws.close()

# Wake word detection using Porcupine
class PorcupineWakeword:
    def __init__(self, keyword_path=None, keywords=KEYWORDS, sensitivity=0.5, callback=None):
        self.callback = callback
        self.porcupine = pvporcupine.create(access_key=PICOVOICE_KEY, keywords=keywords, sensitivities=[sensitivity])
        self.pyaudio_instance = pyaudio.PyAudio()

    async def start_listening(self):
        self.stream = self.pyaudio_instance.open(format=pyaudio.paInt16, channels=1, rate=self.porcupine.sample_rate, input=True, frames_per_buffer=self.porcupine.frame_length)
        print("Listening for wake word...")
        self.is_listening = True
        try:
            while self.is_listening:
                # Read audio data from stream
                audio_data = self.stream.read(self.porcupine.frame_length)
                audio_data = struct.unpack_from("h" * self.porcupine.frame_length, audio_data)
                result = self.porcupine.process(audio_data)
                if False or result >= 0:  # Wake word detected
                    print("Wake word detected")
                    self.stop_listening()
                    if self.callback:
                        await self.callback()
        except Exception as e:
            logger.error(f"Error during wake word detection: {e}")
        print("Done listening for wake word")

    def stop_listening(self):
        self.is_listening = False
        self.stream.stop_stream()
        self.stream.close()
#        self.pyaudio_instance.terminate()

# Voice assistant with wake word detection
class VoiceAssistant:
    def __init__(self, instructions=INSTRUCTIONS, voice=VOICE):
        self.realtime_client = RealtimeClient(instructions, voice)
        self.wakeword_detector = PorcupineWakeword() #callback=self.start_conversation)
        self.listening_active = False

    async def start_conversation(self):
        print("Starting conversation...")

        # Connect to OpenAI realtime API
        await self.realtime_client.run()

        # Loop
        self.realtime_client.audio_handler.start_recording()
        try:
            for i in range(50):
                # Get audio chunk
                chunk = self.realtime_client.audio_handler.record_chunk()
                if chunk:
                    # Send audio chunk
                    print("Listening... " + str(i) + " " + str(len(chunk)))
                    base64_chunk = base64.b64encode(chunk).decode('utf-8')
                    await self.realtime_client.send_event({"type": "input_audio_buffer.append", "audio": base64_chunk})
                else:
                    break
        except Exception as e:
            logger.error(f"Error during audio recording: {e}")
        finally:
            print("Stopped recording")
            self.realtime_client.audio_handler.stop_recording()

        # Done sending audio chunks
        await self.realtime_client.send_event({"type": "input_audio_buffer.commit"})
        print("Sent input_audio_buffer.commit")

        # Ask for response
        await self.realtime_client.send_event({"type": "response.create"})
        print("Sent response.create to ask for a response")

        # Sleep
        await asyncio.sleep(10)

#    async def end_conversation(self):
#        logger.info("Ending conversation...")
#        self.listening_active = False
#        await self.realtime_client.cleanup()

    async def run(self):
        self.listening_active = True
        while self.listening_active:
            try:
                # Listen for wake word
                print("***Listening for wake word")
                await self.wakeword_detector.start_listening()

                # Listen for command and respond
                print("***Starting conversation")
                await self.start_conversation()
            except Exception as e:
                logger.error(f"Done: {e}")

# Run
if __name__ == "__main__":
    assistant = VoiceAssistant()
    asyncio.run(assistant.run())
