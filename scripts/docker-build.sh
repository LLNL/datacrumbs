#!/bin/bash

# DataCrumbs Docker Build and Push Script
# This script builds the DataCrumbs Docker image and optionally pushes it to Docker Hub

set -e

# Configuration
IMAGE_NAME="datacrumbs"
DOCKER_HUB_USERNAME="${DOCKER_HUB_USERNAME:-your-dockerhub-username}"
VERSION="${VERSION:-latest}"
DOCKERFILE_PATH="infrastrutcure/docker/Dockerfile"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the correct directory
if [ ! -f "$DOCKERFILE_PATH" ]; then
    print_error "Dockerfile not found at $DOCKERFILE_PATH"
    print_error "Please run this script from the datacrumbs root directory"
    exit 1
fi

# Function to show usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -u, --username USERNAME    Docker Hub username (default: $DOCKER_HUB_USERNAME)"
    echo "  -v, --version VERSION      Image version tag (default: $VERSION)"
    echo "  -p, --push                 Push to Docker Hub after building"
    echo "  -h, --help                 Show this help message"
    echo ""
    echo "Environment variables:"
    echo "  DOCKER_HUB_USERNAME        Docker Hub username"
    echo "  VERSION                    Image version tag"
    echo ""
    echo "Examples:"
    echo "  $0                         # Build image locally"
    echo "  $0 -u myusername -v v1.0   # Build with specific username and version"
    echo "  $0 -p                      # Build and push to Docker Hub"
}

# Parse command line arguments
PUSH_TO_HUB=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -u|--username)
            DOCKER_HUB_USERNAME="$2"
            shift 2
            ;;
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        -p|--push)
            PUSH_TO_HUB=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Validate Docker Hub username if pushing
if [ "$PUSH_TO_HUB" = true ] && [ "$DOCKER_HUB_USERNAME" = "your-dockerhub-username" ]; then
    print_error "Please provide a valid Docker Hub username with -u or set DOCKER_HUB_USERNAME environment variable"
    exit 1
fi

# Detect container runtime (Docker or Podman)
if command -v podman >/dev/null 2>&1 && ! command -v docker >/dev/null 2>&1; then
    CONTAINER_CMD="podman"
    print_status "Using Podman as container runtime"
elif command -v docker >/dev/null 2>&1; then
    CONTAINER_CMD="docker"
    print_status "Using Docker as container runtime"
else
    print_error "Neither Docker nor Podman found. Please install one of them."
    exit 1
fi

# Image tags
LOCAL_TAG="$IMAGE_NAME:$VERSION"
HUB_TAG="$DOCKER_HUB_USERNAME/$IMAGE_NAME:$VERSION"
HUB_LATEST_TAG="$DOCKER_HUB_USERNAME/$IMAGE_NAME:latest"

print_status "Building DataCrumbs Docker image..."
print_status "Image name: $LOCAL_TAG"
print_status "Dockerfile: $DOCKERFILE_PATH"

# Build the Docker image
if $CONTAINER_CMD build -f "$DOCKERFILE_PATH" -t "$LOCAL_TAG" .; then
    print_status "✅ Docker image built successfully: $LOCAL_TAG"
else
    print_error "❌ Failed to build Docker image"
    exit 1
fi

# Tag for Docker Hub if pushing
if [ "$PUSH_TO_HUB" = true ]; then
    print_status "Tagging image for Docker Hub..."
    
    # Tag with version
    if $CONTAINER_CMD tag "$LOCAL_TAG" "$HUB_TAG"; then
        print_status "✅ Tagged as $HUB_TAG"
    else
        print_error "❌ Failed to tag image for Docker Hub"
        exit 1
    fi
    
    # Tag as latest if version is not already latest
    if [ "$VERSION" != "latest" ]; then
        if $CONTAINER_CMD tag "$LOCAL_TAG" "$HUB_LATEST_TAG"; then
            print_status "✅ Tagged as $HUB_LATEST_TAG"
        else
            print_error "❌ Failed to tag image as latest"
            exit 1
        fi
    fi
    
    print_status "Pushing to Docker Hub..."
    print_warning "Make sure you're logged into Docker Hub (run 'docker login docker.io' for Podman)"
    print_warning "Make sure the repository '$DOCKER_HUB_USERNAME/datacrumbs' exists on Docker Hub"
    
    # Check if we're using podman
    PUSH_CMD="$CONTAINER_CMD push"
    
    # Push version tag
    if $PUSH_CMD "$HUB_TAG"; then
        print_status "✅ Pushed $HUB_TAG"
    else
        print_error "❌ Failed to push $HUB_TAG"
        print_error "Please ensure:"
        print_error "1. You're logged in: docker login docker.io (or podman login docker.io)"
        print_error "2. Repository exists: https://hub.docker.com/r/$DOCKER_HUB_USERNAME/datacrumbs"
        print_error "3. You have push permissions to the repository"
        exit 1
    fi
    
    # Push latest tag if different from version
    if [ "$VERSION" != "latest" ]; then
        if $PUSH_CMD "$HUB_LATEST_TAG"; then
            print_status "✅ Pushed $HUB_LATEST_TAG"
        else
            print_error "❌ Failed to push $HUB_LATEST_TAG"
            exit 1
        fi
    fi
    
    print_status "🎉 Successfully pushed DataCrumbs image to Docker Hub!"
    echo ""
    echo "Your image is now available at:"
    echo "  docker pull $HUB_TAG"
    if [ "$VERSION" != "latest" ]; then
        echo "  docker pull $HUB_LATEST_TAG"
    fi
else
    print_status "🎉 Successfully built DataCrumbs image locally!"
    echo ""
    echo "To run the container:"
    echo "  $CONTAINER_CMD run -it --privileged --cap-add=ALL $LOCAL_TAG"
    echo ""
    echo "To push to Docker Hub, run:"
    echo "  $0 -u <your-dockerhub-username> -p"
fi

echo ""
print_status "Image size:"
$CONTAINER_CMD images "$LOCAL_TAG" --format "table {{.Repository}}\t{{.Tag}}\t{{.Size}}"
