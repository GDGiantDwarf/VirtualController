"""
Simple WebSocket server to test mobile controller connection
Receives button events from the Flutter mobile app and prints them
Also prints a QR code for easy pairing.
"""
import asyncio
import json
import socket
import websockets

# Optional: ASCII QR printing if 'qrcode' is installed
try:
    import qrcode
    _HAS_QR = True
except Exception:
    _HAS_QR = False

def get_local_ip():
    """Get the local IP address of this machine"""
    try:
        hostname = socket.gethostname()
        return socket.gethostbyname(hostname)
    except Exception:
        return "localhost"

def print_pairing_info(ip: str, port: int):
    url = f"ws://{ip}:{port}/controller"
    print(f"\n🔗 Pairing URL: {url}")

    if _HAS_QR:
        try:
            qr = qrcode.QRCode(border=1)
            qr.add_data(url)
            qr.make(fit=True)
            print("\n🟦 Scan this QR with the mobile app:\n")
            qr.print_ascii(invert=True)
        except Exception as e:
            print(f"\n⚠️  Could not render QR: {e}")
    else:
        print("\n⚠️  'qrcode' not installed. Install with: pip install qrcode[pil]")

async def handle_controller(websocket):
    """Handle incoming controller connections"""
    client_address = websocket.remote_address
    print(f"\n✅ Controller connected from {client_address[0]}:{client_address[1]}")

    try:
        async for message in websocket:
            try:
                data = json.loads(message)

                if data.get('type') == 'button':
                    button = data.get('button')
                    pressed = data.get('pressed')
                    status = "PRESSED" if pressed else "RELEASED"
                    print(f"🎮 Button: {button:15s} {status}")

                elif data.get('type') == 'analog':
                    stick = data.get('stick')
                    x = data.get('x', 0)
                    y = data.get('y', 0)
                    print(f"🕹️  Analog {stick:5s}: X={x:+.2f}, Y={y:+.2f}")

                else:
                    print(f"📦 Unknown message: {data}")

            except json.JSONDecodeError:
                print(f"⚠️  Invalid JSON: {message}")

    except websockets.exceptions.ConnectionClosed:
        print(f"\n❌ Controller disconnected from {client_address[0]}:{client_address[1]}")

async def main():
    """Start the WebSocket server"""
    host = "0.0.0.0"
    port = 8765

    local_ip = get_local_ip()

    print("=" * 60)
    print("🎮 Virtual Controller Test Server")
    print("=" * 60)
    print(f"\n📡 Server starting on {local_ip}:{port}")
    print_pairing_info(local_ip, port)
    print(f"\n⏳ Waiting for controller connection...\n")

    async with websockets.serve(handle_controller, host, port):
        await asyncio.Future()  # Run forever

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n\n🛑 Server stopped by user")
