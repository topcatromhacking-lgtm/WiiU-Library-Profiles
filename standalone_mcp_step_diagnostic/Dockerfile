FROM ghcr.io/wiiu-env/devkitppc:20260225 AS build
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts /opt/devkitpro
WORKDIR /project
COPY standalone_mcp_step_diagnostic/ standalone_mcp_step_diagnostic/
RUN make -C standalone_mcp_step_diagnostic && test -s standalone_mcp_step_diagnostic/wlp_mcp_steps_v102.wuhb
FROM scratch AS artifacts
COPY --from=build /project/standalone_mcp_step_diagnostic/wlp_mcp_steps_v102.wuhb /
