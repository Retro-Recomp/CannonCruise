import os
code_dir = 'code'
path = os.path.join(code_dir, 'WorldSpawn.cpp')
with open(path) as f:
    text = f.read()

# Replace ApplyEventTrigger
old_apply = "RwBool ApplyEventTrigger(void *, RwUInt32, const RwUInt8 *) { return TRUE; }"
new_apply = "RwBool ApplyEventTrigger(void *obj, RwUInt32 msg, const RwUInt8 *data) {" \
    " CEventTrigger *trigger = static_cast<CEventTrigger *>(obj);" \
    " if (trigger) {" \
    " CAttributePacket packet;" \
    " return trigger->Apply(packet);" \
    " }" \
    " return TRUE; }"
text = text.replace(old_apply, new_apply)

with open(path, 'w') as f:
    f.write(text)

print('Replaced ApplyEventTrigger')