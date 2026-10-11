"""Local-only village monitor. Requires Python 3, no third-party packages."""
import argparse
import json
import time
import webbrowser
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = Path(__file__).resolve().parents[2]
PAGE = Path(__file__).with_name("index.html")
SOURCE = None

def snapshot():
    candidates = [SOURCE] if SOURCE else list(ROOT.glob("**/monitoring-state.json"))
    candidates = [p for p in candidates if p and p.is_file()]
    if not candidates:
        return {"available": False, "message": "게임을 실행하면 상태가 표시됩니다."}
    path = max(candidates, key=lambda p: p.stat().st_mtime)
    before = path.stat().st_mtime
    data = json.loads(path.read_text(encoding="utf-8"))
    return {"available": True, "age_seconds": max(0, time.time()-before),
            "source": str(path), "data": data}

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        route = self.path.split("?")[0]
        if route == "/api/state":
            try:
                payload = json.dumps(snapshot(), ensure_ascii=False).encode("utf-8")
                code = 200
            except (OSError, ValueError) as exc:
                payload = json.dumps({"available": False, "message": str(exc)}).encode()
                code = 503
            content_type = "application/json; charset=utf-8"
        elif route in ("/", "/index.html"):
            payload, code = PAGE.read_bytes(), 200
            content_type = "text/html; charset=utf-8"
        else:
            self.send_error(404)
            return
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)
    def log_message(self, *args):
        pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()
    SOURCE = args.source
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    url = f"http://127.0.0.1:{args.port}"
    print(f"Village Monitor: {url}\nStop: Ctrl+C", flush=True)
    if not args.no_browser:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()

