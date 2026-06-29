"""Editor init: NavMesh + MCP + GameMode."""
import unreal

unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer")

try:
    us = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    w = us.get_editor_world()
    if not w or w.get_name() == "Untitled":
        unreal.log("[INIT] No level loaded yet")
    else:
        # Add NavMesh volume if missing, set proper size
        nav_exists = False
        for a in unreal.EditorLevelLibrary.get_all_level_actors():
            if isinstance(a, unreal.NavMeshBoundsVolume):
                nav_exists = True
                a.set_actor_location(unreal.Vector(0, 0, 200))
                a.set_actor_scale3d(unreal.Vector(50, 50, 20))
                break
        if not nav_exists:
            v = unreal.EditorLevelLibrary.spawn_actor_from_class(
                unreal.NavMeshBoundsVolume.static_class(),
                unreal.Vector(0, 0, 200))
            v.set_actor_label("NavMeshVolume")
            v.set_actor_scale3d(unreal.Vector(50, 50, 20))
            unreal.log("[INIT] NavMeshVolume added")

        # Set GameMode
        ws = w.get_world_settings()
        gm = str(ws.get_editor_property("DefaultGameMode") or "")
        if "ArenaGameMode" not in gm:
            arena = unreal.load_class(None, "/Script/battle.ArenaGameMode")
            if arena:
                ws.set_editor_property("DefaultGameMode", arena)
                unreal.log("[INIT] GameMode set")

        unreal.EditorLevelLibrary.save_current_level()
        unreal.log("[INIT] Level saved. Reopen editor then PIE.")
except Exception as e:
    unreal.log_warning(f"[INIT] {e}")
