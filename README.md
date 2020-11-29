# Docker container
```
docker run -p 3922:22 -p 8985:8080 -v /dev:/dev -v /opt/MVS:/opt/MVS --name hikvisionbroker-extension --privileged hikvisionbroker-extension:latest 
```

# Pitfall when using MVS as a library
When using 
```
set_target_properties(mainlib PROPERTIES LINK_FLAGS "-Wl,-rpath=/opt/MVS/lib/64 -Wl,--disable-new-dtags")
```
to configure the library and link another executable against this library, everything looks fine 
but the Save Image function will fail for no reason. 

To resolve this, do not use the line in CMake but use LD_LIBRARY_PATH to set the search path.
