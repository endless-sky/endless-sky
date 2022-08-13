# Source Code

The source-code for Endless Sky is split into multiple libraries and programs:
- **[foundation](./foundation)** - Very basic data-structures that only depend on default c++ features.
- **[platform](./platform)** - The layer that connects to OpenGL, SDL and other libraries used by ES. Should be stubbed/mocked when doing unit-tests for other components. Code in the platform layer can depend on code in the foundation layer.
- **[templates](./templates)** - Classes related to serialization and data-only model representation. Should be re-usable for other tools than the main-game. Code in templates can depend on foundation, but should not depend on the platform layer.
- **[runtime](./runtime)** - Classes that implement the in-game runtime functionality. Will have dependencies on templates, foundation and platform. Unit-testing for classes in runtime will require the platform classes to be stubbed/mocked.
- **[maingame](./maingame)** - The main executable for the game. This executable will depend on the compiled runtime, templates, platform and foundation.
