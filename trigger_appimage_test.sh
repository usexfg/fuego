#!/bin/bash

# Script to trigger AppImage workflow for testing
# This creates a test tag to trigger the AppImage build

echo "🚀 Triggering AppImage workflow test..."

# Create a test tag
TEST_TAG="appimage-test-$(date +%Y%m%d-%H%M%S)"
echo "📋 Creating test tag: $TEST_TAG"

# Create and push the tag
git tag "$TEST_TAG"
git push origin "$TEST_TAG"

echo "✅ Test tag created and pushed!"
echo "🔍 AppImage workflow should now be triggered."
echo "📊 Monitor progress at: https://github.com/ColinRitman/fuego/actions"
echo ""
echo "🏷️  Tag created: $TEST_TAG"
echo "🗑️  To cleanup later: git tag -d $TEST_TAG && git push --delete origin $TEST_TAG"