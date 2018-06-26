TEMPLATE=subdirs

SUBDIRS = \
   kernel

!uikit: SUBDIRS += \
   mimetypes \
   plugin \
   serialization \
   statemachine \
   thread \
   tools
