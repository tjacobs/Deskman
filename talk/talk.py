import os
import json
import websocket

# Keys
# export OPENAI_API_KEY="sk-"
OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY")

# OpenAI realitime model API endpoint
url = "wss://api.openai.com/v1/realtime?model=gpt-4o-realtime-preview-2024-12-17"
headers = [
    "Authorization: Bearer " + OPENAI_API_KEY,
    "OpenAI-Beta: realtime=v1"
]

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
            "modalities": [ "text" ]
        }
    }
    ws.send(json.dumps(event))


# Receive messages
def on_message(ws, message):
    server_event = json.loads(message)

#    print("Received event:", json.dumps(server_event, indent=2))

#    if server_event['type'] == "response.text.delta":
#        print(server_event['delta'], end='')

    if server_event['type'] == "response.done":
        print("")
        print(server_event['response']['output'][0]['content'][0]['text'])

    if server_event['type'] == "response.audio.delta":
        # Access Base64-encoded audio chunks:
        print("Audio size:")
        print(len(server_event['delta']))


# Connect
ws = websocket.WebSocketApp(
    url,
    header=headers,
    on_open=on_open,
    on_message=on_message,
)
ws.run_forever()
