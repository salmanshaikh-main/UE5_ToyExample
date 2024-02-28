# Makefile generated by MakefileGenerator.cs
# *DO NOT EDIT*

UNREALROOTPATH = /home/shais0a/Downloads/UnrealEngine-5.3.2-release
GAMEPROJECTFILE =/home/shais0a/Documents/Unreal Projects/Thesis/UE-Multiplayer-Game-To-Hack/ThirdPerson.uproject

TARGETS = \
	ThirdPerson-Linux-Debug  \
	ThirdPerson-Linux-DebugGame  \
	ThirdPerson-Linux-Test  \
	ThirdPerson-Linux-Shipping  \
	ThirdPerson \
	ThirdPersonClient-Linux-Debug  \
	ThirdPersonClient-Linux-DebugGame  \
	ThirdPersonClient-Linux-Test  \
	ThirdPersonClient-Linux-Shipping  \
	ThirdPersonClient \
	ThirdPersonEditor-Linux-Debug  \
	ThirdPersonEditor-Linux-DebugGame  \
	ThirdPersonEditor-Linux-Test  \
	ThirdPersonEditor-Linux-Shipping  \
	ThirdPersonEditor \
	ThirdPersonServer-Linux-Debug  \
	ThirdPersonServer-Linux-DebugGame  \
	ThirdPersonServer-Linux-Test  \
	ThirdPersonServer-Linux-Shipping  \
	ThirdPersonServer \
	UnrealClient-Linux-Debug  \
	UnrealClient-Linux-DebugGame  \
	UnrealClient-Linux-Test  \
	UnrealClient-Linux-Shipping  \
	UnrealClient \
	UnrealEditor-Linux-Debug  \
	UnrealEditor-Linux-DebugGame  \
	UnrealEditor-Linux-Test  \
	UnrealEditor-Linux-Shipping  \
	UnrealEditor \
	UnrealGame-Linux-Debug  \
	UnrealGame-Linux-DebugGame  \
	UnrealGame-Linux-Test  \
	UnrealGame-Linux-Shipping  \
	UnrealGame \
	UnrealServer-Linux-Debug  \
	UnrealServer-Linux-DebugGame  \
	UnrealServer-Linux-Test  \
	UnrealServer-Linux-Shipping  \
	UnrealServer\
	configure

BUILD = bash "$(UNREALROOTPATH)/Engine/Build/BatchFiles/Linux/Build.sh"
PROJECTBUILD = "$(UNREALROOTPATH)/Engine/Binaries/ThirdParty/DotNet/6.0.302/linux/dotnet" "$(UNREALROOTPATH)/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll"

all: StandardSet

RequiredTools: CrashReportClient-Linux-Shipping CrashReportClientEditor-Linux-Shipping ShaderCompileWorker UnrealLightmass EpicWebHelper-Linux-Shipping

StandardSet: RequiredTools UnrealFrontend ThirdPersonEditor UnrealInsights

DebugSet: RequiredTools UnrealFrontend-Linux-Debug ThirdPersonEditor-Linux-Debug


ThirdPerson-Linux-Debug:
	 $(PROJECTBUILD) ThirdPerson Linux Debug  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPerson-Linux-DebugGame:
	 $(PROJECTBUILD) ThirdPerson Linux DebugGame  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPerson-Linux-Test:
	 $(PROJECTBUILD) ThirdPerson Linux Test  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPerson-Linux-Shipping:
	 $(PROJECTBUILD) ThirdPerson Linux Shipping  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPerson:
	 $(PROJECTBUILD) ThirdPerson Linux Development  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonClient-Linux-Debug:
	 $(BUILD) ThirdPersonClient Linux Debug  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonClient-Linux-DebugGame:
	 $(BUILD) ThirdPersonClient Linux DebugGame  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonClient-Linux-Test:
	 $(BUILD) ThirdPersonClient Linux Test  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonClient-Linux-Shipping:
	 $(BUILD) ThirdPersonClient Linux Shipping  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonClient:
	 $(BUILD) ThirdPersonClient Linux Development  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonEditor-Linux-Debug:
	 $(PROJECTBUILD) ThirdPersonEditor Linux Debug  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonEditor-Linux-DebugGame:
	 $(PROJECTBUILD) ThirdPersonEditor Linux DebugGame  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonEditor-Linux-Test:
	 $(PROJECTBUILD) ThirdPersonEditor Linux Test  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonEditor-Linux-Shipping:
	 $(PROJECTBUILD) ThirdPersonEditor Linux Shipping  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonEditor:
	 $(PROJECTBUILD) ThirdPersonEditor Linux Development  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonServer-Linux-Debug:
	 $(BUILD) ThirdPersonServer Linux Debug  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonServer-Linux-DebugGame:
	 $(BUILD) ThirdPersonServer Linux DebugGame  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonServer-Linux-Test:
	 $(BUILD) ThirdPersonServer Linux Test  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonServer-Linux-Shipping:
	 $(BUILD) ThirdPersonServer Linux Shipping  -project="$(GAMEPROJECTFILE)" $(ARGS)

ThirdPersonServer:
	 $(BUILD) ThirdPersonServer Linux Development  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealClient-Linux-Debug:
	 $(BUILD) UnrealClient Linux Debug  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealClient-Linux-DebugGame:
	 $(BUILD) UnrealClient Linux DebugGame  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealClient-Linux-Test:
	 $(BUILD) UnrealClient Linux Test  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealClient-Linux-Shipping:
	 $(BUILD) UnrealClient Linux Shipping  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealClient:
	 $(BUILD) UnrealClient Linux Development  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealEditor-Linux-Debug:
	 $(BUILD) UnrealEditor Linux Debug  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealEditor-Linux-DebugGame:
	 $(BUILD) UnrealEditor Linux DebugGame  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealEditor-Linux-Test:
	 $(BUILD) UnrealEditor Linux Test  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealEditor-Linux-Shipping:
	 $(BUILD) UnrealEditor Linux Shipping  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealEditor:
	 $(BUILD) UnrealEditor Linux Development  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealGame-Linux-Debug:
	 $(BUILD) UnrealGame Linux Debug  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealGame-Linux-DebugGame:
	 $(BUILD) UnrealGame Linux DebugGame  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealGame-Linux-Test:
	 $(BUILD) UnrealGame Linux Test  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealGame-Linux-Shipping:
	 $(BUILD) UnrealGame Linux Shipping  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealGame:
	 $(BUILD) UnrealGame Linux Development  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealServer-Linux-Debug:
	 $(BUILD) UnrealServer Linux Debug  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealServer-Linux-DebugGame:
	 $(BUILD) UnrealServer Linux DebugGame  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealServer-Linux-Test:
	 $(BUILD) UnrealServer Linux Test  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealServer-Linux-Shipping:
	 $(BUILD) UnrealServer Linux Shipping  -project="$(GAMEPROJECTFILE)" $(ARGS)

UnrealServer:
	 $(BUILD) UnrealServer Linux Development  -project="$(GAMEPROJECTFILE)" $(ARGS)

configure:
	xbuild /property:Configuration=Development /verbosity:quiet /nologo "$(UNREALROOTPATH)/Engine/Source/Programs/UnrealBuildTool/UnrealBuildTool.csproj"
	$(PROJECTBUILD) -projectfiles -project="\"$(GAMEPROJECTFILE)\"" -game -engine 

.PHONY: $(TARGETS)
