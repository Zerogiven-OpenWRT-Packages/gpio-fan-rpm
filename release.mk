VERSION_FILE := Makefile

# Hilfsziel: aktuelle Version auslesen
get-version = $(strip $(subst ",,$(shell grep PKG_VERSION $(VERSION_FILE) | cut -d= -f2)))
get-release = $(strip $(subst ",,$(shell grep PKG_RELEASE $(VERSION_FILE) | cut -d= -f2)))

define update-version
	@sed -i 's/^PKG_VERSION:=.*/PKG_VERSION:=$(1)/' $(VERSION_FILE)
endef

define update-release
	@sed -i 's/^PKG_RELEASE:=.*/PKG_RELEASE:=$(1)/' $(VERSION_FILE)
endef

release-major:
	$(eval V := $(call get-version))
	$(eval MAJOR := $(shell echo $(V) | cut -d. -f1))
	$(eval MAJOR := $(shell echo $$(($(MAJOR)+1))))
	$(call update-version,$(MAJOR).0.0)
	$(call update-release,1)
	@git add $(VERSION_FILE)
	@git commit -m "Release version $(MAJOR).$(MINOR).$(PATCH)-r1"
	@git tag "$(MAJOR).$(MINOR).$(PATCH)-r1"
	@echo "Updated to version $(MAJOR).0.0-r1"

release-minor:
	$(eval V := $(call get-version))
	$(eval MAJOR := $(shell echo $(V) | cut -d. -f1))
	$(eval MINOR := $(shell echo $(V) | cut -d. -f2))
	$(eval MINOR := $(shell echo $$(($(MINOR)+1))))
	$(call update-version,$(MAJOR).$(MINOR).0)
	$(call update-release,1)
	@git add $(VERSION_FILE)
	@git commit -m "Release version $(MAJOR).$(MINOR).$(PATCH)-r1"
	@git tag "$(MAJOR).$(MINOR).$(PATCH)-r1"
	@echo "Updated to version $(MAJOR).$(MINOR).0-r1"

release-patch:
	$(eval V := $(call get-version))
	$(eval MAJOR := $(shell echo $(V) | cut -d. -f1))
	$(eval MINOR := $(shell echo $(V) | cut -d. -f2))
	$(eval PATCH := $(shell echo $(V) | cut -d. -f3))
	$(eval PATCH := $(shell echo $$(($(PATCH)+1))))
	$(call update-version,$(MAJOR).$(MINOR).$(PATCH))
	$(call update-release,1)
	@git add $(VERSION_FILE)
	@git commit -m "Release version $(MAJOR).$(MINOR).$(PATCH)-r1"
	@git tag "$(MAJOR).$(MINOR).$(PATCH)-r1"
	@echo "Updated to version $(MAJOR).$(MINOR).$(PATCH)-r1"

release-control:
	$(eval V := $(call get-version))
	$(eval R := $(call get-release))
	$(eval R := $(shell echo $$(($(R)+1))))
	$(call update-release,$(R))
	@git add $(VERSION_FILE)
	@git commit -m "Release version $(V)-r$(R)"
	@git tag "$(V)-r$(R)"
	@echo "Updated to version $(V)-r$(R)"
