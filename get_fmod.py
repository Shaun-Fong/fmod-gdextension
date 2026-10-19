import sys

import requests

argv = sys.argv[1:]

if len(argv) < 4:
    print(
        "Usage: get_fmod.py <user> <password> <platform> <fmod_version>\n"
        "The FMODUSER / FMODPASS credentials are missing. Add both as repository "
        "secrets (Settings > Secrets and variables > Actions)."
    )
    sys.exit(1)

user = argv[0]
password = argv[1]
platform = argv[2]
fmod_version = argv[3]

fmodlink = "https://www.fmod.com/api-login"

if platform == 'linux':
    # linux
    filename = f'fmodstudioapi{fmod_version}linux.tar.gz'
    downloadlink = f'https://www.fmod.com/api-get-download-link?path=files/fmodstudio/api/Linux/&filename=fmodstudioapi{fmod_version}linux.tar.gz&user='
elif platform == 'macos':
    # OS X
    filename = f'fmodstudioapi{fmod_version}osx.dmg'
    downloadlink = f'https://www.fmod.com/api-get-download-link?path=files/fmodstudio/api/Mac/&filename=fmodstudioapi{fmod_version}mac-installer.dmg&user='
elif platform == 'windows':
    # Windows...
    filename = f'fmodstudioapi{fmod_version}win-installer.exe'
    downloadlink = f'https://www.fmod.com/api-get-download-link?path=files/fmodstudio/api/Windows/&filename=fmodstudioapi{fmod_version}win-installer.exe&user='
elif platform == 'android':
    # Android...
    filename = f'fmodstudioapi{fmod_version}android.tar.gz'
    downloadlink = f'https://www.fmod.com/api-get-download-link?path=files/fmodstudio/api/Android/&filename=fmodstudioapi{fmod_version}android.tar.gz&user='
elif platform == 'ios':
    # iOS...
    filename = f'fmodstudioapi{fmod_version}ios.dmg'
    downloadlink = f'https://www.fmod.com/api-get-download-link?path=files/fmodstudio/api/iOS/&filename=fmodstudioapi{fmod_version}ios-installer.dmg&user='
else:
    print(f"Unsupported platform '{platform}'.")
    sys.exit(1)

downloadlink += user

# First login and get a token!
response = requests.post(fmodlink, auth=(user, password))
try:
    payload = response.json()
except ValueError:
    payload = {}
token = payload.get("token")
if not token:
    print(
        f"FMOD login failed (HTTP {response.status_code}). Check the FMODUSER / "
        f"FMODPASS repository secrets: they must be the credentials of a valid "
        f"fmod.com account. Response: {response.text[:200]}"
    )
    sys.exit(1)

print("Received token from FMOD login API.")

# Next request a download link using the token!
response = requests.get(downloadlink, headers={"Authorization": f"Bearer {token}"})
try:
    payload = response.json()
except ValueError:
    payload = {}
url = payload.get("url")
if not url:
    print(
        f"Could not get a download link for platform '{platform}' (HTTP "
        f"{response.status_code}). Response: {response.text[:200]}"
    )
    sys.exit(1)

# Download FMOD!
response = requests.get(url, allow_redirects=True)
if response.status_code != 200 or not response.content:
    print(f"Downloading '{filename}' failed with HTTP {response.status_code}.")
    sys.exit(1)

with open(filename, 'wb') as fmod_archive:
    fmod_archive.write(response.content)

print(f"Downloaded '{filename}' ({len(response.content)} bytes).")
