FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY console_menu_profile_switch/ console_menu_profile_switch/
RUN make -C console_menu_profile_switch && test -s console_menu_profile_switch/wiiu_library_profiles_v081_hash_advisory.wps
FROM scratch AS artifacts
COPY --from=build /project/console_menu_profile_switch/wiiu_library_profiles_v081_hash_advisory.wps /
