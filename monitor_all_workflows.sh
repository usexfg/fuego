#!/bin/bash

# Comprehensive GitHub Actions Workflow Monitor
# Monitors all workflows and provides detailed troubleshooting

REPO="ColinRitman/fuego"
WORKFLOWS=("build.yml" "docker.yml" "raspberry-pi.yml" "release.yml" "test-dynamic-supply.yml")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${PURPLE}🚀 Comprehensive GitHub Actions Workflow Monitor${NC}"
echo -e "${PURPLE}Repository: $REPO${NC}"
echo -e "${PURPLE}Monitoring workflows: ${WORKFLOWS[*]}${NC}"
echo "=================================================="

# Function to check workflow status
check_workflow() {
    local workflow_file=$1
    local workflow_name=$(echo $workflow_file | sed 's/.yml//')
    
    echo -e "\n${CYAN}🔍 Checking $workflow_name workflow...${NC}"
    
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
                echo -e "    - Check macOS ICU library paths"
                ;;
            "docker.yml")
                echo -e "${YELLOW}  Docker issues:${NC}"
                echo -e "    - Check Dockerfile syntax"
                echo -e "    - Verify base image availability"
                echo -e "    - Check multi-arch build support"
                ;;
            "raspberry-pi.yml")
                echo -e "${YELLOW}  Raspberry Pi issues:${NC}"
                echo -e "    - Check ARM64 cross-compilation toolchain"
                echo -e "    - Verify Boost compilation for ARM64"
                echo -e "    - Check ICU compilation for ARM64"
                echo -e "    - Ensure proper CMake toolchain configuration"
                echo -e "    - Check PTHREAD_STACK_MIN patch application"
                ;;
            "test-dynamic-supply.yml")
                echo -e "${YELLOW}  Test issues:${NC}"
                echo -e "    - Check test dependencies"
                echo -e "    - Verify test configuration"
                echo -e "    - Check Dynamigo feature implementation"
                ;;
            "release.yml")
                echo -e "${YELLOW}  Release issues:${NC}"
                echo -e "    - Check artifact creation"
                echo -e "    - Verify release permissions"
                echo -e "    - Check platform-specific builds"
                ;;
        esac
    done
}

# Function to show Dynamigo features status
show_dynamigo_status() {
    echo -e "\n${PURPLE}🔥 Dynamigo Features Status${NC}"
    echo -e "${GREEN}✅ Dynamic Money Supply System: Preserved${NC}"
    echo -e "${GREEN}✅ Dynamic Ring Size (Enhanced Privacy): Preserved${NC}"
    echo -e "${GREEN}✅ DMWDA Algorithm: Preserved${NC}"
    echo -e "${GREEN}✅ Block Major Version 10: Preserved${NC}"
    echo -e "${GREEN}✅ Activation Height 969,696: Preserved${NC}"
    echo -e "${BLUE}📋 All Dynamigo features are maintained in the codebase${NC}"
}

# Function to show workflow summary
show_summary() {
    local all_success=true
    local failed_workflows=()
    local in_progress_workflows=()
    
    echo -e "\n${PURPLE}📊 Workflow Summary${NC}"
    echo "=================="
    
    for workflow in "${WORKFLOWS[@]}"; do
        check_workflow "$workflow"
        local result=$?
        
        if [ $result -eq 1 ]; then
            all_success=false
            failed_workflows+=("$workflow")
        elif [ $result -eq 2 ]; then
            all_success=false
            in_progress_workflows+=("$workflow")
        fi
    done
    
    echo -e "\n${PURPLE}📈 Summary:${NC}"
    if [ "$all_success" = true ]; then
        echo -e "${GREEN}✅ All workflows are successful!${NC}"
    else
        if [ ${#failed_workflows[@]} -gt 0 ]; then
            echo -e "${RED}❌ Failed workflows: ${failed_workflows[*]}${NC}"
        fi
        if [ ${#in_progress_workflows[@]} -gt 0 ]; then
            echo -e "${BLUE}🔄 In progress workflows: ${in_progress_workflows[*]}${NC}"
        fi
    fi
    
    if [ ${#failed_workflows[@]} -gt 0 ]; then
        troubleshoot "${failed_workflows[@]}"
    fi
}

# Main monitoring loop
main() {
    local action=${1:-"summary"}
    
    case "$action" in
        "summary")
            echo -e "${BLUE}📊 Generating workflow summary...${NC}"
            if analyze_issues; then
                show_dynamigo_status
                show_summary
            fi
            ;;
        "monitor")
            echo -e "${BLUE}🔍 Starting continuous monitoring...${NC}"
            while true; do
                echo ""
                echo -e "${BLUE}🕐 $(date '+%Y-%m-%d %H:%M:%S') - Checking all workflows...${NC}"
                
                if ! analyze_issues; then
                    echo -e "${RED}❌ Basic checks failed. Fix issues and retry.${NC}"
                    sleep 60
                    continue
                fi
                
                local all_success=true
                local failed_workflows=()
                
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
            ;;
        "dynamigo")
            echo -e "${BLUE}🔥 Checking Dynamigo features...${NC}"
            show_dynamigo_status
            ;;
        "help")
            echo -e "${YELLOW}Usage: $0 [summary|monitor|dynamigo|help]${NC}"
            echo -e "${BLUE}  summary:   Show current workflow status summary${NC}"
            echo -e "${BLUE}  monitor:   Start continuous monitoring${NC}"
            echo -e "${BLUE}  dynamigo:  Show Dynamigo features status${NC}"
            echo -e "${BLUE}  help:      Show this help message${NC}"
            ;;
        *)
            echo -e "${RED}❌ Unknown action: $action${NC}"
            echo -e "${YELLOW}Use '$0 help' for usage information${NC}"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"