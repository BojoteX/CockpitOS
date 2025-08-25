import json

with open('FA-18C_hornet.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

results = []
for panel_name, controls in data.items():
    for ctrl_name, ctrl in controls.items():
        if ctrl.get("control_type", "").lower() == "metadata":
            results.append({
                "panel": panel_name,
                "identifier": ctrl.get("identifier", ctrl_name),
                "description": ctrl.get("description", ""),
            })

print(f"Total metadata fields found: {len(results)}\n")
for r in results:
    print(f"{r['identifier']:30} | {r['panel']:40} | {r['description']}")
