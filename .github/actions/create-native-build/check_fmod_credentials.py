"""Fail fast when the FMOD account credentials are missing.

The release and pull request workflows download the FMOD Studio API with
``get_fmod.py``, which authenticates against fmod.com with a real account.
Forked repositories never inherit the secrets of the upstream repository, so
without ``FMODUSER`` / ``FMODPASS`` the download dies with an opaque traceback
and every single build job fails before it compiles anything.

This script turns that situation into an explicit check annotation, which is
visible in the Actions UI without opening the (often restricted) job log.
"""

import sys


def main(args):
    user = args[0] if len(args) > 0 else ""
    password = args[1] if len(args) > 1 else ""

    missing = [
        name
        for name, value in (("FMODUSER", user), ("FMODPASS", password))
        if not value
    ]
    if missing:
        subject = " and ".join(missing)
        verb = "are" if len(missing) > 1 else "is"
        print(
            "::error title=Missing FMOD credentials::"
            + subject
            + f" {verb} not set for this repository. Add FMODUSER and FMODPASS as"
            " repository secrets (Settings > Secrets and variables > Actions)"
            " with the credentials of an fmod.com account, then re-run the"
            " workflow. Forked repositories do not inherit upstream secrets."
        )
        return 1

    print("FMOD credentials are configured.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
