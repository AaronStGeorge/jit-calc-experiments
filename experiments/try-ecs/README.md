# `try-ecs`

# Why

ECS + custom compiler should go together like peanut butter and jelly. ECSs can
keep game data we might need to interact with in generated code in nice
[POD](https://en.cppreference.com/w/cpp/named_req/PODType) data types. LLVM/MLIR
understands the layout of POD structs already. Generating and hooking stuff
together should be easy.

# How

Lots of ChatGPT, seems to have slurped up a lot of ECS prior art it's pretty
good at spitting one out.

# What

A "game" I guess that's just some cue balls running around elastically
bouncing off one another.