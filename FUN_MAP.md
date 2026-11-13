{
  "mapping_map": {
    "funs": [
      {"address": "FUN_00401000", "name": "RWS::CEventHandler", "files": ["Actor.h"], "description": ""},
      {"address": "FUN_00401060", "name": "CEventHandler", "files": ["framework/core/eventhandler/ceventhandler.h"], "description": ""}
    ],
    "total_mapped_functions": 372,
    "third_party_libraries": [
      {
        "name": "RwsAssets",
        "include_files": ["code/RwsAssets.h"],
        "cpp_file": "code/RwsAssets.cpp",
        "functions": []
      }
    ],
    "assembly_alignment_notes": [
      "Maintain DAT_00635648 (RenderBehaviorWorld singleton) + DAT_0065d7a4 (RwID strcmp manager)",
      "Preserve memory manager heap layout",
      "Keep global constants in sequence"
    ]
  }
}