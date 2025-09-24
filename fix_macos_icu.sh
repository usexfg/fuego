#!/bin/bash

# Fix macOS ICU Linking Issue
# This script addresses the ICU library linking problem on macOS

echo "🔧 Fixing macOS ICU linking issue..."

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ This script is for macOS only"
    exit 1
fi

# Check if ICU is installed
if ! brew list icu4c@77 >/dev/null 2>&1; then
    echo "📦 Installing ICU4C..."
    brew install icu4c@77
fi

ICU_ROOT=$(brew --prefix icu4c@77)
echo "🔍 ICU Root: $ICU_ROOT"

# Check if ICU libraries exist
if [ ! -f "$ICU_ROOT/lib/libicudata.dylib" ]; then
    echo "❌ ICU data library not found at $ICU_ROOT/lib/libicudata.dylib"
    exit 1
fi

if [ ! -f "$ICU_ROOT/lib/libicui18n.dylib" ]; then
    echo "❌ ICU i18n library not found at $ICU_ROOT/lib/libicui18n.dylib"
    exit 1
fi

if [ ! -f "$ICU_ROOT/lib/libicuuc.dylib" ]; then
    echo "❌ ICU uc library not found at $ICU_ROOT/lib/libicuuc.dylib"
    exit 1
fi

echo "✅ ICU libraries found"

# Test linking
echo "🧪 Testing ICU linking..."
export ICU_ROOT
export PKG_CONFIG_PATH="$ICU_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH"
export LDFLAGS="-L$ICU_ROOT/lib $LDFLAGS"
export CPPFLAGS="-I$ICU_ROOT/include $CPPFLAGS"

# Create a test build directory
mkdir -p build-test-icu
cd build-test-icu

# Configure with ICU
cmake -DCMAKE_BUILD_TYPE=Release \
      -DICU_ROOT="$ICU_ROOT" \
      -DICU_INCLUDE_DIR="$ICU_ROOT/include" \
      -DICU_DATA_LIBRARY="$ICU_ROOT/lib/libicudata.dylib" \
      -DICU_I18N_LIBRARY="$ICU_ROOT/lib/libicui18n.dylib" \
      -DICU_UC_LIBRARY="$ICU_ROOT/lib/libicuuc.dylib" \
      -DCMAKE_PREFIX_PATH="$ICU_ROOT" \
      -DCMAKE_LIBRARY_PATH="$ICU_ROOT/lib" \
      -DCMAKE_INCLUDE_PATH="$ICU_ROOT/include" \
      -G Ninja ..

if [ $? -eq 0 ]; then
    echo "✅ CMake configuration successful"
    
    # Try to build just the Daemon target
    ninja Daemon
    
    if [ $? -eq 0 ]; then
        echo "✅ ICU linking test successful!"
        echo "🎉 The ICU linking issue has been resolved"
    else
        echo "❌ ICU linking test failed"
        echo "🔍 Check the build output above for details"
    fi
else
    echo "❌ CMake configuration failed"
    echo "🔍 Check the configuration output above for details"
fi

cd ..
echo "🏁 ICU linking fix test completed"