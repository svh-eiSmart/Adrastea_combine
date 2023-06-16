KVPFS_DIR = $(KVPFS_ROOT)

KVPFS_SRC_DIR = $(KVPFS_DIR)/src $(KVPFS_DIR)/libcrc/src

INC_DIRS += $(KVPFS_DIR)/inc $(KVPFS_DIR)/libcrc/inc $(KVPFS_DIR)/src

$(eval $(call component_compile_rules,KVPFS))
