#!/bin/bash

# GitHub Actions Monitor Script
# Monitors build status every 2 minutes

REPO="ColinRitman/fuego"
WORKFLOW_NAME="Build and Test"

echo "🔍 Monitoring GitHub Actions for $REPO"
echo "📋 Workflow: $WORKFLOW_NAME"
echo "⏰ Checking every 2 minutes..."
echo "=================================="

while true; do
    echo ""
    echo "🕐 $(date '+%Y-%m-%d %H:%M:%S') - Checking build status..."
    
    # Get the latest workflow run status
    STATUS=$(curl -s "https://api.github.com/repos/$REPO/actions/workflows/build.yml/runs?per_page=1" | jq -r '.workflow_runs[0].status // "unknown"')
    CONCLUSION=$(curl -s "https://api.github.com/repos/$REPO/actions/workflows/build.yml/runs?per_page=1" | jq -r '.workflow_runs[0].conclusion // "unknown"')
    HTML_URL=$(curl -s "https://api.github.com/repos/$REPO/actions/workflows/build.yml/runs?per_page=1" | jq -r '.workflow_runs[0].html_url // "unknown"')
    
    echo "📊 Status: $STATUS"
    echo "🎯 Conclusion: $CONCLUSION"
    echo "🔗 URL: $HTML_URL"
    
    # Check individual job statuses
    RUN_ID=$(curl -s "https://api.github.com/repos/$REPO/actions/workflows/build.yml/runs?per_page=1" | jq -r '.workflow_runs[0].id // "unknown"')
    
    if [ "$RUN_ID" != "unknown" ] && [ "$RUN_ID" != "null" ]; then
        echo "📋 Job Details:"
        curl -s "https://api.github.com/repos/$REPO/actions/runs/$RUN_ID/jobs" | jq -r '.jobs[] | "  \(.name): \(.status) - \(.conclusion // "in_progress")"' 2>/dev/null || echo "  Unable to fetch job details"
    fi
    
    # Status indicators
    case "$STATUS" in
        "completed")
            case "$CONCLUSION" in
                "success")
                    echo "✅ All builds successful!"
                    echo "🎉 Build monitoring complete."
                    break
                    ;;
                "failure")
                    echo "❌ Build failed!"
                    echo "🔧 Check the logs for details: $HTML_URL"
                    ;;
                "cancelled")
                    echo "⏹️ Build was cancelled"
                    ;;
                *)
                    echo "⚠️ Build completed with status: $CONCLUSION"
                    ;;
            esac
            ;;
        "in_progress")
            echo "🔄 Build in progress..."
            ;;
        "queued")
            echo "⏳ Build queued..."
            ;;
        *)
            echo "❓ Unknown status: $STATUS"
            ;;
    esac
    
    echo "⏰ Waiting 2 minutes before next check..."
    sleep 120
done
