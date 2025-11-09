#!/bin/bash

# Enhanced GitHub Actions Workflow Monitor
# Monitors all workflows and provides detailed troubleshooting

REPO="ColinRitman/fuego"
WORKFLOWS=("build.yml" "docker.yml" "raspberry-pi.yml" "release.yml" "test-dynamic-supply.yml")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔍 Enhanced GitHub Actions Monitor for $REPO${NC}"
echo -e "${BLUE}📋 Monitoring workflows: ${WORKFLOWS[*]}${NC}"
echo -e "${BLUE}⏰ Checking every 2 minutes...${NC}"
echo "=================================="

# Function to check workflow status
check_workflow() {
    local workflow_file=$1
    local workflow_name=$(echo $workflow_file | sed 's/.yml//')
    
    echo -e "\n${YELLOW}🔍 Checking $workflow_name workflow...${NC}"
    
    # Get workflow runs
    local runs=$(curl -s "https://api.github.com/repos/$REPO/actions/workflows/$workflow_file/runs?per_page=3")
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ Failed to fetch workflow data for $workflow_name${NC}"
        return 1
    fi
    
    # Parse latest run
    local status=$(echo "$runs" | jq -r '.workflow_runs[0].status // "unknown"')
    local conclusion=$(echo "$runs" | jq -r '.workflow_runs[0].conclusion // "unknown"')
    local html_url=$(echo "$runs" | jq -r '.workflow_runs[0].html_url // "unknown"')
    local created_at=$(echo "$runs" | jq -r '.workflow_runs[0].created_at // "unknown"')
    local run_id=$(echo "$runs" | jq -r '.workflow_runs[0].id // "unknown"')
    
    echo -e "📊 Status: ${status}"
    echo -e "🎯 Conclusion: ${conclusion}"
    echo -e "🕐 Created: ${created_at}"
    echo -e "🔗 URL: ${html_url}"
    
    # Check individual job statuses if run exists
    if [ "$run_id" != "unknown" ] && [ "$run_id" != "null" ]; then
        echo -e "📋 Job Details:"
        local jobs=$(curl -s "https://api.github.com/repos/$REPO/actions/runs/$run_id/jobs")
        if [ $? -eq 0 ]; then
            echo "$jobs" | jq -r '.jobs[] | "  \(.name): \(.status) - \(.conclusion // "in_progress")"' 2>/dev/null || echo "  Unable to fetch job details"
        else
            echo "  Unable to fetch job details"
        fi
    fi
    
    # Status indicators
    case "$status" in
        "completed")
            case "$conclusion" in
                "success")
                    echo -e "${GREEN}✅ $workflow_name workflow successful!${NC}"
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
            return 2
            ;;
        "queued")
            echo -e "${YELLOW}⏳ $workflow_name workflow queued...${NC}"
            return 2
            ;;
        *)
            echo -e "${RED}❓ $workflow_name workflow unknown status: $status${NC}"
            return 1
            ;;
    esac
}

# Function to trigger a workflow run
trigger_workflow() {
    local workflow_file=$1
    echo -e "${BLUE}🚀 Triggering $workflow_file workflow...${NC}"
    
    # This would require a GitHub token with workflow permissions
    # For now, we'll just show the manual trigger URL
    echo -e "${YELLOW}Manual trigger: https://github.com/$REPO/actions/workflows/$workflow_file${NC}"
}

# Function to analyze common issues
analyze_issues() {
    echo -e "\n${BLUE}🔍 Analyzing common issues...${NC}"
    
    # Check if jq is installed
    if ! command -v jq &> /dev/null; then
        echo -e "${RED}❌ jq is not installed. Install with: sudo apt-get install jq${NC}"
        return 1
    fi
    
    # Check if curl is working
    if ! curl -s "https://api.github.com" > /dev/null; then
        echo -e "${RED}❌ Cannot reach GitHub API. Check internet connection.${NC}"
        return 1
    fi
    
    echo -e "${GREEN}✅ Basic checks passed${NC}"
}

# Function to provide troubleshooting suggestions
troubleshoot() {
    local failed_workflows=("$@")
    
    echo -e "\n${YELLOW}🔧 Troubleshooting suggestions:${NC}"
    
    for workflow in "${failed_workflows[@]}"; do
        case "$workflow" in
            "build.yml")
                echo -e "${YELLOW}  Build issues:${NC}"
                echo -e "    - Check CMake configuration"
                echo -e "    - Verify dependencies are installed"
                echo -e "    - Check for compilation errors"
                echo -e "    - Ensure tests are properly configured"
                ;;
            "docker.yml")
                echo -e "${YELLOW}  Docker issues:${NC}"
                echo -e "    - Check Dockerfile syntax"
                echo -e "    - Verify base image availability"
                echo -e "    - Check multi-arch build support"
                ;;
            "test-dynamic-supply.yml")
                echo -e "${YELLOW}  Test issues:${NC}"
                echo -e "    - Check test dependencies"
                echo -e "    - Verify test configuration"
                echo -e "    - Check Dynamigo feature implementation"
                ;;
        esac
    done
}

# Main monitoring loop
main() {
    local all_success=true
    local failed_workflows=()
    
    while true; do
        echo ""
        echo -e "${BLUE}🕐 $(date '+%Y-%m-%d %H:%M:%S') - Checking all workflows...${NC}"
        
        # Analyze basic issues first
        if ! analyze_issues; then
            echo -e "${RED}❌ Basic checks failed. Fix issues and retry.${NC}"
            sleep 120
            continue
        fi
        
        # Check each workflow
        for workflow in "${WORKFLOWS[@]}"; do
            check_workflow "$workflow"
            local result=$?
            
            if [ $result -eq 1 ]; then
                all_success=false
                failed_workflows+=("$workflow")
            elif [ $result -eq 2 ]; then
                all_success=false
            fi
        done
        
        # Summary
        echo -e "\n${BLUE}📊 Summary:${NC}"
        if [ "$all_success" = true ]; then
            echo -e "${GREEN}✅ All workflows are successful!${NC}"
            echo -e "${GREEN}🎉 Monitoring complete.${NC}"
            break
        else
            echo -e "${RED}❌ Some workflows are failing or in progress${NC}"
            if [ ${#failed_workflows[@]} -gt 0 ]; then
                troubleshoot "${failed_workflows[@]}"
            fi
        fi
        
        echo -e "${BLUE}⏰ Waiting 2 minutes before next check...${NC}"
        sleep 120
    done
}

# Run main function
main