FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY console_menu_save_attestation/ console_menu_save_attestation/
RUN make -C console_menu_save_attestation && test -s console_menu_save_attestation/wiiu_library_profiles_v132_save_attest.wps
FROM scratch AS artifacts
COPY --from=build /project/console_menu_save_attestation/wiiu_library_profiles_v132_save_attest.wps /
