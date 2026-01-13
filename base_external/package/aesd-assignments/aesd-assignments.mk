##############################################################
#
# AESD-ASSIGNMENTS
#
##############################################################

AESD_ASSIGNMENTS_VERSION = 'abc22cb6e573c2cf7c57c27a2c84019da0193325'
AESD_ASSIGNMENTS_SITE = 'git@github.com:cu-ecen-aeld/assignments-3-and-later-happysmaran.git'
AESD_ASSIGNMENTS_SITE_METHOD = git
AESD_ASSIGNMENTS_GIT_SUBMODULES = YES
	
define AESD_ASSIGNMENTS_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/finder-app all
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/server all
	$(MAKE) -C $(LINUX_DIR) ARCH=arm64 CROSS_COMPILE="$(TARGET_CROSS)" M=$(@D)/aesd-char-driver modules
endef

define AESD_ASSIGNMENTS_INSTALL_TARGET_CMDS
	# 1. Install finder-app and assignment 4 configuration
	$(INSTALL) -d 0755 $(TARGET_DIR)/etc/finder-app/conf/
	$(INSTALL) -m 0755 $(@D)/conf/* $(TARGET_DIR)/etc/finder-app/conf/
	$(INSTALL) -m 0755 $(@D)/assignment-autotest/test/assignment4/* $(TARGET_DIR)/bin

	# 2. Install aesdsocket server
	$(INSTALL) -m 0755 -D $(@D)/server/aesdsocket $(TARGET_DIR)/usr/bin/aesdsocket

	# 3. Install Init script (S99aesdsocket)
	$(INSTALL) -m 0755 -D $(@D)/aesd-char-driver/S99aesdsocket.sh $(TARGET_DIR)/etc/init.d/S99aesdsocket

	# 4. Install Driver and Load/Unload scripts as actual files (No Symlinks!)
	$(INSTALL) -m 0755 -D $(@D)/aesd-char-driver/aesdchar.ko $(TARGET_DIR)/usr/bin/aesdchar.ko
	$(INSTALL) -m 0755 -D $(@D)/aesd-char-driver/aesdchar_load $(TARGET_DIR)/usr/bin/aesdchar_load
	$(INSTALL) -m 0755 -D $(@D)/aesd-char-driver/aesdchar_unload $(TARGET_DIR)/usr/bin/aesdchar_unload

	# 5. Install Assignment 9 Test Scripts
	$(INSTALL) -m 0755 $(@D)/server/sockettest.sh $(TARGET_DIR)/usr/bin/sockettest.sh
	$(INSTALL) -m 0755 $(@D)/assignment-autotest/test/assignment9/drivertest.sh $(TARGET_DIR)/usr/bin/drivertest.sh
	$(INSTALL) -m 0755 $(@D)/assignment-autotest/test/assignment9/assignment-test.sh $(TARGET_DIR)/usr/bin/assignment-test.sh

	# 6. Patch the test scripts to use absolute paths for load/unload.
	# This replaces "./aesdchar_load" with "/usr/bin/aesdchar_load" in the scripts.
	$(SED) 's|\./aesdchar_|/usr/bin/aesdchar_|g' $(TARGET_DIR)/usr/bin/drivertest.sh
	$(SED) 's|\./aesdchar_|/usr/bin/aesdchar_|g' $(TARGET_DIR)/usr/bin/sockettest.sh
	$(SED) 's|\./aesdchar_|/usr/bin/aesdchar_|g' $(TARGET_DIR)/usr/bin/assignment-test.sh

	# 7. Compatibility: Ensure modprobe can find the module (fixes modules.dep error)
	mkdir -p $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/extra/
	cp $(@D)/aesd-char-driver/aesdchar.ko $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/extra/
	
	ln -sf /bin/mktemp $(TARGET_DIR)/usr/bin/tempfile
endef

$(eval $(generic-package))
