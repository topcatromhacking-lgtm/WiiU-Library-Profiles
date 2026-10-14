FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY console_menu_profile_preview/ console_menu_profile_preview/
RUN make -C console_menu_profile_preview && test -s console_menu_profile_preview/wiiu_library_profiles_v12_menu_preview.wps
FROM scratch AS artifacts
COPY --from=build /project/console_menu_profile_preview/wiiu_library_profiles_v12_menu_preview.wps /
