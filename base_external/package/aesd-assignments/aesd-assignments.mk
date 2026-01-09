
##############################################################
#
# AESD-ASSIGNMENTS
#
##############################################################

#TODO: Fill up the contents below in order to reference your assignment 3 git contents
AESD_ASSIGNMENTS_VERSION = bf0022e1f92eb8bda83f60f65c83f2d0c3ba9160
# Note: Be sure to reference the *ssh* repository URL here (not https) to work properly
# with ssh keys and the automated build/test system.
# Your site should start with git@github.com:
AESD_ASSIGNMENTS_SITE = git@github.com:cu-ecen-aeld/assignments-3-and-later-happysmaran.git
AESD_ASSIGNMENTS_SITE_METHOD = git
AESD_ASSIGNMENTS_GIT_SUBMODULES = YES

define AESD_ASSIGNMENTS_BUILD_CMDS
    # Build the socket server
    $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/server all

    # Build the kernel module using the full path to the cross-compiler
    $(MAKE) -C $(LINUX_DIR) \
        ARCH=arm64 \
        CROSS_COMPILE=$(TARGET_CROSS) \
        M=$(@D)/aesd-char-driver modules
endef

define AESD_ASSIGNMENTS_INSTALL_TARGET_CMDS
    # Install socket server
    $(INSTALL) -m 0755 $(@D)/server/aesdsocket $(TARGET_DIR)/usr/bin/
    
    # Install driver and scripts
    $(INSTALL) -m 0755 $(@D)/aesd-char-driver/aesdchar.ko $(TARGET_DIR)/usr/bin/
    $(INSTALL) -m 0755 $(@D)/aesd-char-driver/aesdchar_load $(TARGET_DIR)/usr/bin/
    $(INSTALL) -m 0755 $(@D)/aesd-char-driver/aesdchar_unload $(TARGET_DIR)/usr/bin/
    
    # Install the Init script
    $(INSTALL) -m 0755 $(@D)/aesd-char-driver/S99aesdsocket.sh $(TARGET_DIR)/etc/init.d/S99aesdsocket
    
    # Keep your old finder-app stuff if you need it for the autotester
    $(INSTALL) -d 0755 $(TARGET_DIR)/etc/finder-app/conf/
    $(INSTALL) -m 0755 $(@D)/conf/* $(TARGET_DIR)/etc/finder-app/conf/
    $(INSTALL) -m 0755 $(@D)/assignment-autotest/test/assignment8/* $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
