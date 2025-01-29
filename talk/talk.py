import os
import json
import websocket

# Keys
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
    data = json.loads(message)
    print("Received event:", json.dumps(data, indent=2))

# Connect
ws = websocket.WebSocketApp(
    url,
    header=headers,
    on_open=on_open,
    on_message=on_message,
)
ws.run_forever()
