# Services

The engine has a services system. Each service should have a single concrete class that inherits from `SERVICE` (see
`src/libs/core/include/service.h`) and implements its methods.

If the service exposes a virtual base/interface class, define the constexpr static `ServiceName` member on that base
class. Otherwise, define it on the concrete class. Ex.:

```
class MyService : public SERVICE {
public:
    constexpr static const char* ServiceName = "MyService";

    // Rest of the service implementation
    // ...
};
```

In the service source file, you can register the service with the `CREATE_SERVICE` macro. Ex.:

```
CREATE_SERVICE(MyService)
```

To retrieve a service instance, the `GetService` function on Core can be used.
Previously you would specify the service name as a string literal. Now, you can use the `GetServiceX` function to
retrieve the service instance by passing the requested service class as template parameter.

```
static_cast<MyService *>(core.GetService("MyService"));
// now becomes
auto* my_service = GetServiceX<MyService>();
```

## Service list (overview)

Below is a non-exhaustive list of services registered in this repo and their intended purpose.

- AnimationServiceImp (`AnimationService`): Creates and manages animation objects for models.
- GEOMETRY (`VGEOMETRY`): Loads/creates geometry and related animation data; provides geometry rendering helpers.
- COLL (`COLLIDE`): Collision queries and spatial tests (ray tracing, clipping, local collide objects).
- PCS_CONTROLS (`CONTROLS`): Input/control mapping and state tracking for user/system controls.
- STRSERVICE (`VSTRSERVICE`): String/localization service (language selection and string lookups).
- DX9RENDER (`VDX9RENDER`): DirectX9 renderer and device management, drawing, textures, fonts, etc.
- LostDeviceSentinel (`SERVICE`): Monitors DX9 device loss/reset and pauses frame processing when needed.
- SoundService (`VSoundService`): Sound playback, mixing, and 3D audio control.
- ParticleService (`IParticleService`): Particle system manager creation and access.

