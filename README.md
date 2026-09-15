# mdprev

A simple and clean Markdown previewer.

## Features

- Fully supports GitHub-style markdown syntax
    - MathJax
    - Images
    - Inline HTML
    - All other formatting options
- Can be opened from other devices on the same LAN
- Hot-reloads page automatically on Markdown change
- Event-driven for maximum responsiveness and resource efficiency

## Installation

1. First pull Git submodules and v1.x.x CForge header by running `./repo-init.sh`
2. Build and install the project by executing `./cforge.h release install`

## Usage

Run `mdprev <file>` to start hosting a webserver with port `12345`. Open your preferred browser and navigate to the page generated: `http://localhost:12345`.

### Example

Run

```sh
mdprev README.md
```

in this folder to preview this README.
