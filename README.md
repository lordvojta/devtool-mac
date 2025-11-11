<h1>Tool</h1>

# from your project root (where /build/devtool exists)
sudo cp build/devtool /usr/local/bin/devtool


<hr>

# Generate UUID
devtool uuid

# Slugify text
devtool slugify "Firemní systém na míru"

# Convert case formats
devtool case kebab|snake|camel|pascal "Input Text"

# URL encode / decode
devtool url encode "žluťoučký kůň"
devtool url decode "%C5%A1"

# Base64 encode / decode
devtool b64 encode "secret"
devtool b64 decode "c2VjcmV0"

# Check missing .env keys
devtool env check .env.example .env

# Bump version in package.json
devtool version bump patch package.json

# JSON pretty-print / minify
devtool json pretty package.json
devtool json minify package.json > package.min.json
# devtool-mac
