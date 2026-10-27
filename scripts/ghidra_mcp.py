"""Minimal client for the GhidrAssistMCP MCP server (HTTP, JSON-RPC).

Read-only: the module only calls query tools
(list_*, get_*). It refuses any other tool name, to comply with the
"Ghidra is read-only" rule of CLAUDE.md.

Usage:
    from ghidra_mcp import Ghidra
    g = Ghidra()
    print(g.call("list_functions", program_name="/happy.exe", limit=10))
"""
import json
import urllib.request

URL = "http://127.0.0.1:8080/mcp"
READ_ONLY_PREFIXES = ("list_", "get_", "search_")


class Ghidra:
    def __init__(self, url=URL):
        self.url = url
        self.session = None
        self.next_id = 1
        r = self._rpc("initialize", {
            "protocolVersion": "2025-03-26",
            "capabilities": {},
            "clientInfo": {"name": "happy-decomp-inventory", "version": "1"},
        })
        self.server = r.get("serverInfo")
        self._notify("notifications/initialized")

    def _post(self, payload):
        data = json.dumps(payload).encode()
        req = urllib.request.Request(self.url, data=data, method="POST")
        req.add_header("Content-Type", "application/json")
        req.add_header("Accept", "application/json, text/event-stream")
        if self.session:
            req.add_header("Mcp-Session-Id", self.session)
        with urllib.request.urlopen(req, timeout=300) as resp:
            sid = resp.headers.get("Mcp-Session-Id")
            if sid:
                self.session = sid
            body = resp.read().decode("utf-8")
            ctype = resp.headers.get("Content-Type", "")
        if not body.strip():
            return None
        if "text/event-stream" in ctype:
            # Keep the last "data:" message that contains a response.
            msgs = [json.loads(line[5:]) for line in body.splitlines()
                    if line.startswith("data:") and line[5:].strip()]
            return msgs[-1] if msgs else None
        return json.loads(body)

    def _rpc(self, method, params):
        i = self.next_id
        self.next_id += 1
        r = self._post({"jsonrpc": "2.0", "id": i, "method": method,
                        "params": params})
        if r is None:
            raise RuntimeError("empty response for %s" % method)
        if "error" in r:
            raise RuntimeError("%s: %s" % (method, r["error"]))
        return r["result"]

    def _notify(self, method):
        self._post({"jsonrpc": "2.0", "method": method})

    def call(self, tool, **args):
        """Call a query tool and return the text of its response."""
        if not tool.startswith(READ_ONLY_PREFIXES):
            raise ValueError("tool refused (read-only): %s" % tool)
        r = self._rpc("tools/call", {"name": tool, "arguments": args})
        parts = [c.get("text", "") for c in r.get("content", [])
                 if c.get("type") == "text"]
        text = "\n".join(parts)
        if r.get("isError"):
            raise RuntimeError("%s(%s): %s" % (tool, args, text))
        return text
