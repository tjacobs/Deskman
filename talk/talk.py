import os
import json
import websocket
import base64
import sounddevice as sd
import numpy as np
from queue import Queue
from threading import Thread

# Keys
# export OPENAI_API_KEY="sk-"
OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY")

# OpenAI realitime model API endpoint
url = "wss://api.openai.com/v1/realtime?model=gpt-4o-realtime-preview-2024-12-17"
headers = [
    "Authorization: Bearer " + OPENAI_API_KEY,
    "OpenAI-Beta: realtime=v1"
]

# Audio buffer queue and playback state
audio_queue = Queue()
is_playing = False

def audio_player():
    global is_playing
    is_playing = True
    
    while is_playing:
        # Collect chunks until we have some audio
        chunks = []
        total_samples = 0
        while total_samples < 24000:  # Buffer ~1 second of audio
            try:
                chunk = audio_queue.get(timeout=0.1)
                chunks.append(chunk)
                total_samples += len(chunk)
            except:
                if chunks:  # Play what we have if queue is empty
                    break
                continue
        if chunks:
            # Concatenate chunks and play
            audio_data = np.concatenate(chunks)
            sd.play(audio_data, samplerate=24000)
            sd.wait()

# Afer opening, send messages
def on_open(ws):
    print("Connected to server.")

    # Update session
    event = {
        "type": "session.update",
        "session": {
            "instructions": "Never use the word 'dog' in your responses!"
        }
    }
    print(event['session']['instructions'])
    ws.send(json.dumps(event))

    # Ask a question
    event = {
        "type": "conversation.item.create",
        "item": {
            "type": "message",
            "role": "user",
            "content": [
                {
                    "type": "input_text",
                    "text": "What animal goes woof?",
                }
            ]
        }
    }
    print(event['item']['content'][0]['text'])
    ws.send(json.dumps(event))

    # Ask it to respond
    event = {
        "type": "response.create",
        "response": {
            "modalities": [ "audio", "text" ]
        }
    }
    ws.send(json.dumps(event))


# Receive messages
def on_message(ws, message):
    server_event = json.loads(message)

#    print("Received event:", json.dumps(server_event, indent=2))

    if server_event['type'] == "response.text.delta":
        print(server_event['delta'], end='')

    if server_event['type'] == "error":
        print("*******")
        print(server_event['error']['message'])
        print("*******")

    if server_event['type'] == "response.done":
        print("Response: ")
        print(server_event['response']['output'][0]['content'][0]['transcript'])

    if server_event['type'] == "response.audio.delta":
        # Decode base64 audio data
        audio_bytes = base64.b64decode(server_event['delta'])
        
        # Convert bytes to numpy array
        audio_array = np.frombuffer(audio_bytes, dtype=np.int16)
        
        # Add to queue instead of playing directly
        audio_queue.put(audio_array)
        
        # Start player thread if not already running
        global is_playing
        if not is_playing:
            Thread(target=audio_player, daemon=True).start()


# Connect
ws = websocket.WebSocketApp(
    url,
    header=headers,
    on_open=on_open,
    on_message=on_message,
)
ws.run_forever()
