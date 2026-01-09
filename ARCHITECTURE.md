# Game Architecture

This document describes the modular architecture of the game engine.

## Overview

The codebase has been refactored from a monolithic `main.c` into a clean, modular architecture with clear separation of concerns.

## Directory Structure

```
src/
├── main.c                 # Entry point - simple initialization and cleanup
├── config.h              # Centralized configuration constants
│
├── engine/               # Core engine management
│   ├── engine.h
│   └── engine.c         # Main game loop, initialization, cleanup
│
├── graphics/             # Rendering and graphics
│   ├── camera.h/c        # Camera logic and view/projection matrices
│   ├── renderer.h/c      # OpenGL state management
│   ├── state.h/c         # OpenGL state utilities
│   └── shader.h/c        # Shader compilation and management
│
├── window/               # Window management
│   ├── window.h
│   └── window.c          # GLFW window creation and callbacks
│
├── math/                 # Math utilities
│   ├── math.h
│   └── math.c            # Matrix operations (4x4 matrices)
│
├── world/                # Game world entities
│   ├── terrain.c         # Terrain generation and rendering
│   ├── water.c           # Water rendering
│   └── skybox.c          # Skybox rendering
│
├── gui/                  # User interface
│   ├── gui.h
│   └── gui.c             # FPS counter and UI rendering
│
└── [other modules]       # file_ops, texture, noise, etc.
```

## Module Responsibilities

### Engine (`engine/`)
- **Purpose**: Core game loop and system coordination
- **Responsibilities**:
  - Initialize all subsystems (window, graphics, world, GUI)
  - Run main game loop (update, render)
  - Coordinate between systems
  - Cleanup on shutdown

### Graphics (`graphics/`)
- **Camera**: First-person camera with position, rotation, and view/projection matrices
- **Renderer**: OpenGL state setup and clearing
- **State**: OpenGL state management utilities (blend, depth, culling)
- **Shader**: Shader compilation and uniform management

### Window (`window/`)
- **Purpose**: GLFW window abstraction
- **Responsibilities**:
  - Window creation and configuration
  - Callback management
  - Cursor mode control
  - Window size queries

### Math (`math/`)
- **Purpose**: Mathematical utilities
- **Functions**:
  - 4x4 matrix operations (identity, multiply, rotate, translate)
  - Perspective projection matrix generation

### World (`world/`)
- **Purpose**: Game world entities
- **Components**:
  - Terrain: LOD-based terrain generation and rendering
  - Water: Instanced water quad rendering
  - Skybox: Cubemap skybox rendering

### GUI (`gui/`)
- **Purpose**: User interface rendering
- **Features**:
  - FPS counter display
  - Nuklear-based UI system

## Data Flow

```
main.c
  └─> engine_init()
       ├─> window_create()
       ├─> camera_init()
       ├─> renderer_set_opengl_state()
       ├─> engine_setup_world()  (terrain, water, skybox)
       └─> engine_setup_gui()
  
  └─> engine_run()
       ├─> camera_process_input()
       ├─> camera_update_view()
       ├─> camera_update_projection()
       ├─> terrain_lod_manager_update()
       ├─> skybox_render()
       ├─> terrain_lod_manager_render()
       ├─> water_render_gl()
       └─> gui_render_fps()
  
  └─> engine_cleanup()
```

## Key Design Decisions

1. **Separation of Concerns**: Each module has a single, well-defined responsibility
2. **Dependency Direction**: Lower-level modules (math, window) don't depend on higher-level ones (engine)
3. **Configuration Centralization**: All constants in `config.h`
4. **Minimal main.c**: Entry point is now just 10 lines
5. **State Management**: OpenGL state changes are abstracted through the state module

## Benefits

- **Maintainability**: Easy to locate and modify specific functionality
- **Testability**: Modules can be tested independently
- **Readability**: Clear structure makes code easier to understand
- **Extensibility**: New features can be added without touching existing code
- **Debugging**: Issues are easier to isolate to specific modules

## Adding New Features

1. **New Graphics Feature**: Add to `graphics/` module
2. **New World Entity**: Add to `world/` module
3. **New Math Utility**: Add to `math/` module
4. **New System**: Create new module directory and integrate in `engine.c`
