# ODIN Voice Unreal C++ Sample

![Odin Header](/Documentation/odin_header.jpg)

This project accompanies the [corresponding guide of the ODIN Voice Unreal C++ Developer Documentation of 4Players](https://docs.4players.io/voice/unreal/guides/unreal-cpp-guide/).

The project mainly consists of C++ classes that show a simple implementation of the ODIN voice chat plugin by 4Players for Unreal Engine. Follow the link to the guide for more information.

The Unreal C++ Sample uses the bundled ODIN Unreal Plugin **20208.0.0** and **Unreal Engine 5.8**.

## Configuring the Access Key

1. Open `Content/Odin/Blueprints/BP_OdinCPPPlayerController` in the Blueprint editor.
2. Select the inherited **OdinClient** component.
3. In its Details panel, set **Odin > Authentication > Access Key** to your ODIN access key.

The sample character automatically calls `ConnectToOdin` when its player ID is assigned or replicated. Configure the key in the component defaults before playing, or set it before that automatic connection. For an explicit connection or reconnect, use the Blueprint-callable **Connect to Odin** node with the player's GUID. **Disconnect from Odin** stops audio and leaves the room.

**Production security:** Using an access-key for generating tokens on the client is for development and testing use only. Do not commit real keys or ship them in client assets. Generate room tokens on a trusted backend for production instead.


## Accessing Other Versions

Specific versions are available via the following branches and tags:

* **Latest Unreal Plugin** Use the `main` branch for the latest version of the ODIN Voice Unreal C++ Sample, currently using Unreal Plugin 20208.0.0 and Unreal Engine 5.8.
* **Unreal Plugin v1.x** For accessing the ODIN Voice Unreal C++ Sample, that's compatible with v1.x of the Unreal Plugin, please switch to the `v1.x` branch.

## What is ODIN Voice?

ODIN is a cross-platform software development kit (SDK) that enables developers to integrate real-time chat technology into multiplayer games, apps, and websites.

For more information, please visit [the ODIN Voice Chat website](https://odin.4players.io/voice-chat/).