import os
import json
os.chdir(os.path.dirname(os.path.abspath(__file__)))

with open("FA-18C_hornet.json", "r", encoding="utf-8") as f:
    data = json.load(f)

ifei = data["Integrated Fuel/Engine Indicator (IFEI)"]

print(f"{'Identifier':<20} | {'Length':<6} | {'Description':<35} | {'Likely Match'}")
print('-'*90)

for key, val in ifei.items():
    identifier = key
    # Find string length if present (try to infer from outputs if not present)
    maxlen = ""
    description = ""
    likely = ""
    # Check if it's a string/label/texture/etc
    if "outputs" in val:
        for out in val["outputs"]:
            if "max_length" in out:
                maxlen = out["max_length"]
            elif "max_value" in out and isinstance(out["max_value"], int):
                # If string, infer by digits, else leave blank
                if out["max_value"] > 9 and out["max_value"] < 100:
                    maxlen = 2
                elif out["max_value"] >= 100 and out["max_value"] < 1000:
                    maxlen = 3
                elif out["max_value"] >= 1000:
                    maxlen = len(str(out["max_value"]))
                else:
                    maxlen = 1
            if "description" in out and not description:
                description = out["description"]
    if not maxlen and "max_length" in val:
        maxlen = val["max_length"]
    if not maxlen and "max_value" in val:
        maxlen = len(str(val["max_value"]))
    if not description and "description" in val:
        description = val["description"]

    print(f"{identifier:<20} | {str(maxlen):<6} | {description[:35]:<35} | ")

