import qbs

Project {
    references: [
        "accessibility",
        "clipboard",
        "devicediscovery",
        "edid",
        "eglconvenience",
        "eventdispatchers",
        "fbconvenience",
        "fontdatabases",
        "glxconvenience",
        "graphics",
        "input",
        "kmsconvenience",
        "linuxaccessibility",
        "platformcompositor",
        "services",
        "themes",
        // "windowsuiautomation",
    ]
}

/*
if(unix:!darwin)|qtConfig(xcb): \
     SUBDIRS += services

 qtConfig(opengl): \
@@ -33,12 +34,14 @@
         SUBDIRS += linuxaccessibility
         linuxaccessibility.depends += accessibility
     }
+    win32:!winrt: SUBDIRS += windowsuiautomation
 }


qtConfig(vulkan): \
    SUBDIRS += vkconvenience
*/
