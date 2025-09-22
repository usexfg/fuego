#!/bin/bash

# Enhanced GitHub Actions Monitor Script
# Monitors all workflow builds and provides detailed status
# Usage: ./monitor_github_actions.sh [repo] [check_interval_minutes]

REPO=${1:-"ColinRitman/fuego"}
CHECK_INTERVAL=${2:-2}
MONITOR_ALL_WORKFLOWS=true

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔍 Enhanced GitHub Actions Monitor${NC}"
echo -e "${BLUE}📋 Repository: $REPO${NC}"
echo -e "${BLUE}⏰ Check interval: $CHECK_INTERVAL minutes${NC}"
if [ "$MONITOR_ALL_WORKFLOWS" = true ]; then
    echo -e "${BLUE}📋 Monitoring all workflows${NC}"
else
    echo -e "${BLUE}📋 Monitoring main build workflow${NC}"
fi
echo "=================================="

# Function to check API rate limit
check_rate_limit() {
    local rate_limit=$(curl -s "https://api.github.com/rate_limit" | jq -r '.rate.remaining // "0"')
    if [ "$rate_limit" -lt 10 ]; then
        echo -e "${RED}⚠️ API rate limit low: $rate_limit requests remaining${NC}"
        return 1
    fi
    return 0
}

# Function to get workflow status
get_workflow_status() {
    local workflow_file=$1
    local workflow_info=$(curl -s "https://api.github.com/repos/$REPO/actions/workflows/$workflow_file/runs?per_page=5" 2>/dev/null)
    echo "$workflow_info"
}

# Function to display workflow status
display_workflow_status() {
    local workflow_file=$1
    local workflow_name=$2
    local workflow_data=$(get_workflow_status "$workflow_file")

    if [ $? -ne 0 ] || [ "$workflow_data" = "" ]; then
        echo -e "${YELLOW}⚠️ Could not fetch status for $workflow_name${NC}"
        return 1
    fi

    local status=$(echo "$workflow_data" | jq -r '.workflow_runs[0].status // "unknown"')
    local conclusion=$(echo "$workflow_data" | jq -r '.workflow_runs[0].conclusion // "unknown"')
    local html_url=$(echo "$workflow_data" | jq -r '.workflow_runs[0].html_url // "unknown"')
    local run_id=$(echo "$workflow_data" | jq -r '.workflow_runs[0].id // "unknown"')

    echo -e "${PURPLE}📋 $workflow_name:${NC}"
    echo -e "   📊 Status: $status"
    echo -e "   🎯 Conclusion: $conclusion"
    echo -e "   🔗 URL: $html_url"

    # Show job details if available
    if [ "$run_id" != "unknown" ] && [ "$run_id" != "null" ] && [ "$status" = "in_progress" ]; then
        local job_data=$(curl -s "https://api.github.com/repos/$REPO/actions/runs/$run_id/jobs" 2>/dev/null)
        if [ $? -eq 0 ] && [ "$job_data" != "" ]; then
            echo "   📋 Job Details:"
            echo "$job_data" | jq -r '.jobs[] | "      \(.name): \(.status) - \(.conclusion // "in_progress")"' 2>/dev/null || echo "      Unable to fetch job details"
        fi
    fi

    # Status indicators
    if [ "$status" = "completed" ]; then
        case "$conclusion" in
            "success")
                echo -e "   ${GREEN}✅ Build successful!${NC}"
                return 0
                ;;
            "failure")
                echo -e "   ${RED}❌ Build failed!${NC}"
                return 1
                ;;
            "cancelled")
                echo -e "   ${YELLOW}⏹️ Build was cancelled${NC}"
                return 2
                ;;
            *)
                echo -e "   ${YELLOW}⚠️ Build completed with status: $conclusion${NC}"
                return 3
                ;;
        esac
    elif [ "$status" = "in_progress" ]; then
        echo -e "   ${BLUE}🔄 Build in progress...${NC}"
        return 4
    elif [ "$status" = "queued" ]; then
        echo -e "   ${YELLOW}⏳ Build queued...${NC}"
        return 5
    else
        echo -e "   ${YELLOW}❓ Unknown status: $status${NC}"
        return 6
    fi
}

# Main monitoring loop
main_loop() {
    local all_successful=true
    local failed_workflows=()
    local successful_workflows=()
    local in_progress_workflows=()

    while true; do
        echo ""
        echo -e "${BLUE}🕐 $(date '+%Y-%m-%d %H:%M:%S') - Checking build status...${NC}"

        # Check rate limit
        if ! check_rate_limit; then
            echo -e "${RED}⏸️ Pausing due to API rate limit...${NC}"
            sleep 300  # Wait 5 minutes
            continue
        fi

        # Reset status
        all_successful=true
        failed_workflows=()
        successful_workflows=()
        in_progress_workflows=()

        # Monitor specific workflows
        workflows=(
            "build.yml:Main Build"
            "testnet.yml:Testnet Build"
            "test-dynamic-supply.yml:Dynamic Supply Test"
            "docker.yml:Docker Images"
        )

        for workflow_info in "${workflows[@]}"; do
            IFS=':' read -r workflow_file workflow_name <<< "$workflow_info"
            local result=$(display_workflow_status "$workflow_file" "$workflow_name")

            case $? in
                0)  # Success
                    successful_workflows+=("$workflow_name")
                    ;;
                1)  # Failure
                    all_successful=false
                    failed_workflows+=("$workflow_name")
                    ;;
                4)  # In progress
                    in_progress_workflows+=("$workflow_name")
                    all_successful=false
                    ;;
                5)  # Queued
                    in_progress_workflows+=("$workflow_name")
                    all_successful=false
                    ;;
            esac
        done

        # Display summary
        echo ""
        echo -e "${BLUE}📊 Summary:${NC}"

        if [ ${#successful_workflows[@]} -gt 0 ]; then
            echo -e "${GREEN}✅ Successful: ${successful_workflows[*]}${NC}"
        fi

        if [ ${#in_progress_workflows[@]} -gt 0 ]; then
            echo -e "${BLUE}🔄 In Progress: ${in_progress_workflows[*]}${NC}"
        fi

        if [ ${#failed_workflows[@]} -gt 0 ]; then
            echo -e "${RED}❌ Failed: ${failed_workflows[*]}${NC}"
        fi

        # Check if all builds are successful
        if [ "$all_successful" = true ] && [ ${#successful_workflows[@]} -gt 0 ]; then
            echo ""
            echo -e "${GREEN}🎉 All monitored workflows are successful!${NC}"
            echo -e "${GREEN}🎊 Build monitoring complete.${NC}"
            break
        fi

        echo ""
        echo -e "${BLUE}⏰ Waiting $CHECK_INTERVAL minutes before next check...${NC}"
        sleep $((CHECK_INTERVAL * 60))
    done
}

# Run main loop
main_loop
