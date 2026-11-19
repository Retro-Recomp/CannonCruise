import os
code_dir = 'code'
path = os.path.join(code_dir, 'WorldSpawn.cpp')
with open(path) as f:
    text = f.read()

# Find and replace CreateEventTrigger
old_create = "void *CreateEventTrigger() { return new CEventTriggerStub; }"
new_create = "void *CreateEventTrigger() { return new CEventTrigger; }"
text = text.replace(old_create, new_create)

with open(path, 'w') as f:
    f.write(text)

print('Replaced CreateEventTrigger')