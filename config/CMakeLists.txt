
install(FILES systemd/aktualizr-ubuntu.service
        DESTINATION lib/systemd/system
        RENAME aktualizr.service
        PERMISSIONS OWNER_READ OWNER_WRITE
        COMPONENT aktualizr)

install(FILES sota_ubuntu.toml
        DESTINATION lib/sota
        RENAME sota.toml
        PERMISSIONS OWNER_READ OWNER_WRITE
        COMPONENT aktualizr)

install(FILES secondary/virtualsec.json
        DESTINATION lib/sota
        RENAME demo_secondary.json
        PERMISSIONS OWNER_READ OWNER_WRITE
        COMPONENT aktualizr)

install(DIRECTORY DESTINATION /etc/sota/conf.d COMPONENT aktualizr)
install(DIRECTORY DESTINATION lib/sota/conf.d COMPONENT aktualizr)
