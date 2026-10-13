FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY standalone_no_sd_diagnostic/ standalone_no_sd_diagnostic/
RUN make -C standalone_no_sd_diagnostic && test -s standalone_no_sd_diagnostic/wlp_no_sd_v103.wuhb
FROM scratch AS artifacts
COPY --from=build /project/standalone_no_sd_diagnostic/wlp_no_sd_v103.wuhb /
