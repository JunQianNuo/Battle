"""
Register exec_python tool with UE MCP ToolsetRegistry.
This allows the MCP HTTP server to execute arbitrary Python in the editor.
"""
import unreal

try:
    from toolset_registry import toolset_registry

    @toolset_registry.tool_call(
        name="exec_python",
        description="Execute Python code in the UE editor. Returns output or error.",
        params={
            "code": {"type": "string", "description": "Python code to execute"}
        }
    )
    def exec_python(code: str) -> str:
        import io, sys, traceback
        buf = io.StringIO()
        old = sys.stdout
        sys.stdout = buf
        try:
            exec(code)
            return buf.getvalue() or "OK"
        except Exception as e:
            return f"ERROR: {e}\n{traceback.format_exc()}"
        finally:
            sys.stdout = old

    unreal.log("[toolset_exec] Registered exec_python tool")
except Exception as e:
    unreal.log_warning(f"[toolset_exec] {e}")
