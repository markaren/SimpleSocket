
import asyncio
import ssl
import websockets

async def main():
    host = "127.0.0.1"
    port = 443
    uri = f"wss://{host}:{port}"

    # Trust system CAs (public or enterprise)
    ssl_ctx = ssl.create_default_context()

    # Last resort for ad‑hoc testing only
    ssl_ctx.check_hostname = False
    ssl_ctx.verify_mode = ssl.CERT_NONE

    async with websockets.connect(uri, ssl=ssl_ctx) as ws:

        msg = await asyncio.wait_for(ws.recv(), timeout=5)
        print(f"<- recv: {msg}")
        await ws.send("hello")
        print("-> sent: hello")
        await ws.close()

if __name__ == "__main__":
    asyncio.run(main())