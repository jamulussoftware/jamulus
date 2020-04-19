# -*- coding: utf-8 -*-
import biplist
import os.path


def icon_from_app(app_path):
    plist_path = os.path.join(app_path, "Contents", "Info.plist")
    plist = biplist.readPlist(plist_path)
    icon_name = plist["CFBundleIconFile"]
    icon_root, icon_ext = os.path.splitext(icon_name)
    icon_name = icon_root + (icon_ext or ".icns")

    return os.path.join(app_path, "Contents", "Resources", icon_name)


def validate_key_path(key, example):
    value = defines.get(key, None)

    if value is None:
        raise ValueError("The " + key + " key must be specified.\n"
                         "Example: dmgbuild -D " + key + "=" + example + " ...")

    if not os.path.exists(value):
        raise ValueError("The " + key + " key must be a valid path.")

    return value


# Path of the applications to deploy
app_path = validate_key_path("app_path", "Jamulus.app")
server_path = validate_key_path("server_path", "JamulusServer.app")

# Name of the applications to deploy
app_name = os.path.basename(app_path)
server_name = os.path.basename(server_path)

# Volume format (see hdiutil create -help)
format = defines.get("format", "UDBZ")

# Volume size
size = defines.get('size', None)

# Files to include
files = [
    app_path,
    server_path
]

# Symlinks to create
symlinks = { 'Applications': '/Applications' }

# Background
background = validate_key_path("background", "picture.png")

# Volume icon
badge_icon = icon_from_app(app_path)

# Select the default view
default_view = "icon-view"

# Set these to True to force inclusion of icon/list view settings
include_icon_view_settings = False
include_list_view_settings = False

# Where to put the icons
icon_locations = {
    app_name: (630, 210),
    server_name: (530, 210),
    "Applications": (820, 210)
}

# View/Window element configuration
show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False
show_icon_preview = False

# Window position in ((x, y), (w, h)) format
window_rect = ((200, 400), (900, 320))

# Icon view configuration
arrange_by = None
grid_offset = (0, 0)
grid_spacing = 72
scroll_position = (0, 0)
label_pos = "bottom"
icon_size = 72
text_size = 12

# License configuration
license = {
    "default-language": "en_US",
    "licenses": { "en_US": validate_key_path("license", "COPYING") }
}
