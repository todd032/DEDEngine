# Codex Cloud Web Build Trial

Use the official Codex Cloud environment settings to create a dedicated web-build environment for this repo.

Recommended settings:

- Name: `dedengine-web-build`
- Base image: `universal`
- Setup script: `bash codex/cloud/setup-web-build.sh`
- Maintenance script: `bash codex/cloud/maintenance-web-build.sh`
- Agent internet access: `Off`

This follows the Codex Cloud guidance that environments are configured in settings, setup scripts can install project-specific dependencies, and agent internet access is off by default. See:

- [Configure environments](https://developers.openai.com/codex/cloud/environments)
- [Internet access](https://developers.openai.com/codex/cloud/internet-access)

After the environment is created, Codex Cloud can validate the same command used locally and in GitHub Actions:

```bash
bash tools/build_web.sh
```
