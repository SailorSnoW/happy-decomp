"""Minimal client for the HTTP API of GhidraMCP 7.0.0 (Ghidra 12.1.4, port 8089).

Read-only: at startup, the client reads /mcp/schema and only allows the
GET endpoints marked read_only (which excludes save_program,
save_all_programs and switch_program). Any other call is refused, to
comply with the "Ghidra is read-only" rule of CLAUDE.md.

Usage:
    from ghidra_rest import Ghidra
    g = Ghidra("happy.exe")
    g.get("find_functions", limit=0)["functions"]
    g.get("get_functions", functions="0x0040A970,0x0040AB30",
          fields="signature,callers,callees,decompiled_code",
          include_call_context="false")
"""
import json
import urllib.parse
import urllib.request

URL = "http://127.0.0.1:8089/"


class Ghidra:
    def __init__(self, program, url=URL):
        self.program = program
        self.url = url
        schema = self._fetch("mcp/schema")
        self.allowed = {t["path"].lstrip("/") for t in schema["tools"]
                        if t["method"] == "GET" and t.get("read_only")}

    def _fetch(self, path, query=None):
        u = self.url + path
        if query:
            u += "?" + urllib.parse.urlencode(query)
        with urllib.request.urlopen(u, timeout=300) as resp:
            return json.loads(resp.read().decode("utf-8"))

    def get(self, tool, **args):
        """Call a query endpoint and return the decoded JSON."""
        if tool not in self.allowed:
            raise ValueError("tool refused (read-only): %s" % tool)
        args.setdefault("program", self.program)
        r = self._fetch(tool, args)
        if isinstance(r, dict) and "error" in r:
            raise RuntimeError("%s(%s): %s" % (tool, args, r["error"]))
        return r

    def get_all(self, tool, key, page=500, **args):
        """Walk through all pages (limit/offset) and concatenate the `key` list."""
        out, offset = [], 0
        while True:
            r = self.get(tool, limit=page, offset=offset, **args)
            out.extend(r[key])
            offset += page
            if offset >= r.get("total", 0):
                return out
