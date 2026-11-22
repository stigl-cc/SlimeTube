# Contributing to SlimeTube
Thank you for considering contributing to slimetube!  
Here are some rules(and some advice) you should follow if you decide to contribute:  

## Development setup
1. Follow [the run guide](README.md#normally) from README.md up to step 4  
2. After configuring via `cmake -B build`, there will be a `build/compile_commands.json` file. You can configure your editor/ide to use it as the compilation database  
3. You can use `data/queries/insert_samples.sql` to easilly create test user and videos for testing purposes  
4. Then you can use `cmake --build build --config Release -j<number-of-cpu-threads> && build/slimytube` for subsequent testing and recompilation  

## Pull Requests
Please follow the existing code style and conventions, in general please use:  
1. Indenting with 4 spaces, not tabs  
2. Do not auto-break code lines, only manually and when it makes the code cleaner/more readable (e.g., when defining the properties of a struct in emplace_back)  
3. Do not use more code than somewhat neceseary for the problem at hand if it doesn't make the code more readable/performant (a bugfix does not need 400 lines)  
4. You may not use computer generated code if you do not understand it and cannot explain what it does  
5. Make sure your pull requests compile  

## Issues
Don't be afraid to create issues, just make sure to include  
1. English  
2. Steps to reproduce  
3. Anything special about your environment  
4. Any other details you feel would be important  
You should also engage in your own issue, not abandon it immediately after its creation.  
