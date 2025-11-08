#!/bin/bash

# Trigger and Monitor GitHub Actions Workflows
# This script triggers workflows and monitors their progress

REPO="ColinRitman/fuego"
GITHUB_TOKEN="${GITHUB_TOKEN:-}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🚀 GitHub Actions Workflow Trigger & Monitor${NC}"
echo -e "${BLUE}Repository: $REPO${NC}"
echo "=================================="

# Function to trigger workflow
trigger_workflow() {
    local workflow_file=$1
    local workflow_name=$(echo $workflow_file | sed 's/.yml//')
    
    echo -e "${YELLOW}🚀 Triggering $workflow_name workflow...${NC}"
    
    if [ -z "$GITHUB_TOKEN" ]; then
        echo -e "${YELLOW}⚠️ No GitHub token provided. Manual trigger required.${NC}"
        echo -e "${BLUE}Manual trigger URL: https://github.com/$REPO/actions/workflows/$workflow_file${NC}"
        return 1
    fi
    
    # Trigger workflow via API
    local response=$(curl -s -X POST \
        -H "Authorization: token $GITHUB_TOKEN" \
        -H "Accept: application/vnd.github.v3+json" \
        "https://api.github.com/repos/$REPO/actions/workflows/$workflow_file/dispatches" \
        -d '{"ref":"master"}')
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ $workflow_name workflow triggered successfully${NC}"
        return 0
    else
        echo -e "${RED}❌ Failed to trigger $workflow_name workflow${NC}"
        echo -e "${YELLOW}Response: $response${NC}"
        return 1
    fi
}

# Function to monitor workflow runs
monitor_workflow() {
    local workflow_file=$1
    local workflow_name=$(echo $workflow_file | sed 's/.yml//')
    local max_wait_time=1800  # 30 minutes
    local check_interval=30   # 30 seconds
    local elapsed=0
    
    echo -e "${BLUE}🔍 Monitoring $workflow_name workflow...${NC}"
    
    while [ $elapsed -lt $max_wait_time ]; do
        local runs=$(curl -s "https://api.github.com/repos/$REPO/actions/workflows/$workflow_file/runs?per_page=1")
        
        if [ $? -ne 0 ]; then
            echo -e "${RED}❌ Failed to fetch workflow status${NC}"
            return 1
        fi
        
        local status=$(echo "$runs" | jq -r '.workflow_runs[0].status // "unknown"')
        local conclusion=$(echo "$runs" | jq -r '.workflow_runs[0].conclusion // "unknown"')
        local html_url=$(echo "$runs" | jq -r '.workflow_runs[0].html_url // "unknown"')
        local created_at=$(echo "$runs" | jq -r '.workflow_runs[0].created_at // "unknown"')
        
        echo -e "${BLUE}⏰ $(date '+%H:%M:%S') - Status: $status, Conclusion: $conclusion${NC}"
        
        case "$status" in
            "completed")
                case "$conclusion" in
                    "success")
                        echo -e "${GREEN}✅ $workflow_name workflow completed successfully!${NC}"
                        echo -e "${BLUE}🔗 URL: $html_url${NC}"
                        return 0
                        ;;
                    "failure")
                        echo -e "${RED}❌ $workflow_name workflow failed!${NC}"
                        echo -e "${YELLOW}🔧 Check logs: $html_url${NC}"
                        return 1
                        ;;
                    "cancelled")
                        echo -e "${YELLOW}⏹️ $workflow_name workflow was cancelled${NC}"
                        return 1
                        ;;
                    *)
                        echo -e "${YELLOW}⚠️ $workflow_name workflow completed with status: $conclusion${NC}"
                        return 1
                        ;;
                esac
                ;;
            "in_progress")
                echo -e "${BLUE}🔄 $workflow_name workflow in progress...${NC}"
                ;;
            "queued")
                echo -e "${YELLOW}⏳ $workflow_name workflow queued...${NC}"
                ;;
            *)
                echo -e "${RED}❓ $workflow_name workflow unknown status: $status${NC}"
                return 1
                ;;
        esac
        
        sleep $check_interval
        elapsed=$((elapsed + check_interval))
    done
    
    echo -e "${RED}⏰ Timeout waiting for $workflow_name workflow to complete${NC}"
    return 1
}

# Function to run local build test
test_local_build() {
    echo -e "${BLUE}🔨 Testing local build...${NC}"
    
    # Create build directory
    mkdir -p build-test
    cd build-test
    
    # Configure
    echo -e "${YELLOW}📋 Configuring with CMake...${NC}"
    if cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF ..; then
        echo -e "${GREEN}✅ CMake configuration successful${NC}"
    else
        echo -e "${RED}❌ CMake configuration failed${NC}"
        return 1
    fi
    
    # Build
    echo -e "${YELLOW}🔨 Building...${NC}"
    if cmake --build . --parallel; then
        echo -e "${GREEN}✅ Build successful${NC}"
    else
        echo -e "${RED}❌ Build failed${NC}"
        return 1
    fi
    
    cd ..
    echo -e "${GREEN}✅ Local build test completed${NC}"
    return 0
}

# Main function
main() {
    local action=${1:-"monitor"}
    
    case "$action" in
        "trigger")
            echo -e "${BLUE}🚀 Triggering workflows...${NC}"
            trigger_workflow "build.yml"
            ;;
        "monitor")
            echo -e "${BLUE}🔍 Monitoring workflows...${NC}"
            monitor_workflow "build.yml"
            ;;
        "test")
            echo -e "${BLUE}🔨 Testing local build...${NC}"
            test_local_build
            ;;
        "full")
            echo -e "${BLUE}🚀 Full workflow: test, trigger, monitor${NC}"
            test_local_build
            if [ $? -eq 0 ]; then
                trigger_workflow "build.yml"
                if [ $? -eq 0 ]; then
                    monitor_workflow "build.yml"
                fi
            fi
            ;;
        *)
            echo -e "${YELLOW}Usage: $0 [trigger|monitor|test|full]${NC}"
            echo -e "${BLUE}  trigger: Trigger a workflow run${NC}"
            echo -e "${BLUE}  monitor: Monitor current workflow runs${NC}"
            echo -e "${BLUE}  test:    Test local build${NC}"
            echo -e "${BLUE}  full:    Test, trigger, and monitor${NC}"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"