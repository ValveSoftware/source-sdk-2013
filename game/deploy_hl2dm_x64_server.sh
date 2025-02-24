#!/bin/bash

# Font properties
BOLD='\033[1m'
NORMAL='\033[0m'
RED='\033[0;31m'
GREEN='\033[0;32m'

SERVER_DIR_NAME="${1:-hl2dm_clean}"
TF2_DIR_NAME="${2:-tf2_clean}"
SERVER_DIR="$HOME/$SERVER_DIR_NAME/"
SRCDS_LINUX64="srcds_linux64"
SRCDS_RUN_64="srcds_run_64"

install_game() {
    local app_id=$1
    local dir_name=$2
    echo -e "\n${GREEN}${BOLD}Installing app $app_id to $dir_name${NORMAL}\n"
    ./steamcmd.sh +force_install_dir "$dir_name/" +login anonymous +app_update "$app_id" validate -arch 64 +quit || {
        echo -e "${RED}${BOLD}Failed to install app $app_id${NORMAL}"
        exit 1
    }
}

if [ "$(uname -m)" != "x86_64" ]; then
    echo -e "${RED}${BOLD}This script is intended for 64-bit systems only.${NORMAL}"
    exit 1
fi

# Check dependencies
command -v wget >/dev/null 2>&1 || { echo >&2 "wget is required but not installed. Aborting."; exit 1; }
command -v tar >/dev/null 2>&1 || { echo >&2 "tar is required but not installed. Aborting."; exit 1; }

cd "$HOME" || { echo "Error changing directory."; exit 1; }

echo "This script will prepare a template for running a custom HL2DM server."
echo "The server will be installed in $SERVER_DIR."

# Download and extract SteamCMD
wget http://media.steampowered.com/client/steamcmd_linux.tar.gz || { echo -e "${RED}${BOLD}Failed to download steamcmd${NORMAL}"; exit 1; }
trap "rm -f steamcmd_linux.tar.gz" EXIT
tar xvfz steamcmd_linux.tar.gz || { echo -e "${RED}${BOLD}Failed to extract steamcmd${NORMAL}"; exit 1; }

# Install HL2DM x64
install_game 232370 "$SERVER_DIR_NAME"
# Install TF2 x64
install_game 232250 "$TF2_DIR_NAME"

# Modify LD_LIBRARY_PATH
echo -e "\n${GREEN}${BOLD}Modifying and saving LD_LIBRARY_PATH...${NORMAL}\n"
# Check if the path is already in LD_LIBRARY_PATH
if [[ ":$LD_LIBRARY_PATH:" != *":$SERVER_DIR/bin/linux64:"* ]]; then
    # If the path is missing, add it
    export LD_LIBRARY_PATH="$SERVER_DIR/bin/linux64:$LD_LIBRARY_PATH"
    echo "export LD_LIBRARY_PATH=\"$SERVER_DIR/bin/linux64:$LD_LIBRARY_PATH\"" >> ~/.bashrc
fi

# Create symlink for steamclient.so
STEAMCLIENT_DLL_PATH="$HOME/.steam/sdk64/steamclient.so"
if [[ -L  "$STEAMCLIENT_DLL_PATH" ]]; then
    echo -e "${GREEN}${BOLD}Deleting existing steamclient.so file...${NORMAL}"
    rm "$STEAMCLIENT_DLL_PATH" || { echo -e "${RED}${BOLD}Error deleting steamclient.so file${NORMAL}"; exit 1; }
fi
echo -e "\n${GREEN}${BOLD}Creating symbolic link for steamclient.so...${NORMAL}\n"
mkdir -p "$HOME/.steam/sdk64"
ln -s "$HOME/linux64/steamclient.so" "$HOME/.steam/sdk64/"

# Copy 64-bit launcher
cp "$TF2_DIR_NAME/$SRCDS_LINUX64" "$SERVER_DIR" || { echo -e "${RED}${BOLD}Failed to copy file $SRCDS_LINUX64${NORMAL}"; exit 1; }
cp "$TF2_DIR_NAME/$SRCDS_RUN_64" "$SERVER_DIR" || { echo -e "${RED}${BOLD}Failed to copy file $SRCDS_RUN_64${NORMAL}"; exit 1; }

# Make the launcher executable
echo -e "${GREEN}${BOLD}Making $SRCDS_RUN_64 executable...${NORMAL}"
chmod +x "$TF2_DIR_NAME/$SRCDS_RUN_64" || { echo -e "${RED}${BOLD}Failed to change permissions for $SRCDS_RUN_64${NORMAL}"; exit 1; }

echo -e "\n${GREEN}${BOLD}Script completed successfully at $(date)${NORMAL}"
