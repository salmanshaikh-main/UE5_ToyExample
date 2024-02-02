## Deploy 
1. Clone the repo
2. Build the project
   See [Linux Quick start](https://docs.unrealengine.com/5.2/en-US/linux-development-quickstart-for-unreal-engine/#:~:text=Build%20a%20Project%20In%20Unreal,cook%2C%20and%20package%20your%20project.)
   See the [Build operations documentation](https://docs.unrealengine.com/5.2/en-US/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine/) for more details.
   Should be something similar (not sure about the command)
   ```
   [UE Root Directory]/Engine/Build/BatchFiles/RunUAT[.sh|.command] BuildCookRun -project=ThirdPerson.uproject -noP4 -clientconfig=Development -serverconfig=Development -utf8output -platform=[Mac|Linux] -server -serverplatform=[Mac|Linux] -targetplatform=[Mac|Linux] -build -cook -unversionedcookedcontent -compressed -stage -package  
   ```
4. Launch the project:
  ```
  ./Binaries/Win64/ThirdPersonServer.exe -log
  ./Binaries/Win64/ThirdPersonClient.exe 127.0.0.1:7777 -WINDOWED -ResX=800 -ResY=450 -log -Path="/optional/path/to/scenario/file/"
  ```


## Add a scenario
Create a json file following this structure:
```
{
    "Move0":{
        "Timestamp":13.0,
        "Duration":10,
        "X":1000,
        "Y":1210
    },
    "Move1":{
        "Timestamp":34.0,
        "Duration":10,
        "X":500,
        "Y":500
    },
    "Move2":{
        "Timestamp":65.0,
        "Duration":10,
        "X":900,
        "Y":1110
    }
}
```
Then, use the command line argument Path, when launching the client you want to automate:
```
./Binaries/Win64/ThirdPersonMPClient.exe 127.0.0.1:7777 -WINDOWED -ResX=800 -ResY=450 -log -Path=[Path of your json file]
```
