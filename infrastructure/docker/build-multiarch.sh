#!/bin/bash

# Multi-architecture Docker build script for DataCrumbs
# Builds for linux/amd64 and linux/arm64 platforms

set -e

# Configuration
IMAGE_NAME="${IMAGE_NAME:-datacrumbs}"
IMAGE_TAG="${IMAGE_TAG:-latest}"
DOCKERFILE="${DOCKERFILE:-infrastructure/docker/Dockerfile.build}"
REGISTRY="${REGISTRY:-docker.io}"
USERNAME="${USERNAME:-hdevarajan92}"
PLATFORMS="${PLATFORMS:-linux/amd64,linux/arm64}"

# Parse command line arguments
PUSH=false
LOAD=false
RUN=false

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --push              Push the built images to the registry"
    echo "  --load              Load the image into local Docker (only works for single platform)"
    echo "  --run               Run the container after loading (implies --load)"
    echo "  --platform ARCH     Comma-separated list of platforms (default: linux/amd64,linux/arm64)"
    echo "  --tag TAG           Image tag (default: latest)"
    echo "  --name NAME         Image name (default: datacrumbs)"
    echo "  --registry REG      Registry URL (default: docker.io)"
    echo "  --username USER     Registry username (default: hdevarajan92)"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --push                                    # Build and push multi-arch images"
    echo "  $0 --load --platform linux/amd64             # Build and load amd64 only"
    echo "  $0 --run --platform linux/amd64              # Build, load and run container"
    echo "  $0 --push --tag v1.0.0 --name myimage        # Custom tag and name"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --push)
            PUSH=true
            shift
            ;;
        --load)
            LOAD=true
            shift
            ;;
        --run)
            RUN=true
            LOAD=true
            shift
            ;;
        --platform)
            PLATFORMS="$2"
            shift 2
            ;;
        --tag)
            IMAGE_TAG="$2"
            shift 2
            ;;
        --name)
            IMAGE_NAME="$2"
            shift 2
            ;;
        --registry)
            REGISTRY="$2"
            shift 2
            ;;
        --username)
            USERNAME="$2"
            shift 2
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Construct full image name
if [ "$REGISTRY" = "docker.io" ]; then
    FULL_IMAGE_NAME="${USERNAME}/${IMAGE_NAME}:${IMAGE_TAG}"
else
    FULL_IMAGE_NAME="${REGISTRY}/${USERNAME}/${IMAGE_NAME}:${IMAGE_TAG}"
fi

echo "======================================"
echo "Multi-Architecture Docker Build"
echo "======================================"
echo "Image: ${FULL_IMAGE_NAME}"
echo "Platforms: ${PLATFORMS}"
echo "Dockerfile: ${DOCKERFILE}"
echo "Push: ${PUSH}"
echo "Load: ${LOAD}"
echo "======================================"
echo ""

# Check if Docker Buildx is available
if ! docker buildx version &> /dev/null; then
    echo "Error: Docker Buildx is not available. Please install Docker Buildx."
    exit 1
fi

# Create a new builder instance if it doesn't exist
BUILDER_NAME="datacrumbs-multiarch-builder"
if ! docker buildx inspect "$BUILDER_NAME" &> /dev/null; then
    echo "Creating new buildx builder: $BUILDER_NAME"
    docker buildx create --name "$BUILDER_NAME" --use --platform "$PLATFORMS"
else
    echo "Using existing buildx builder: $BUILDER_NAME"
    docker buildx use "$BUILDER_NAME"
fi

# Bootstrap the builder
echo "Bootstrapping builder..."
docker buildx inspect --bootstrap

# Build command
BUILD_CMD="docker buildx build"
BUILD_CMD+=" --platform ${PLATFORMS}"
BUILD_CMD+=" -t ${FULL_IMAGE_NAME}"
BUILD_CMD+=" -f ${DOCKERFILE}"

if [ "$PUSH" = true ] && [ "$LOAD" = true ]; then
    echo "Error: Cannot use --push and --load together. Use --load for single platform only."
    exit 1
fi

if [ "$PUSH" = true ]; then
    BUILD_CMD+=" --push"
    echo "Images will be pushed to registry after build."
elif [ "$LOAD" = true ]; then
    BUILD_CMD+=" --load"
    echo "Image will be loaded into local Docker."
    if [[ "$PLATFORMS" == *","* ]]; then
        echo "Warning: --load only works with a single platform. Building first platform only."
        PLATFORMS=$(echo "$PLATFORMS" | cut -d',' -f1)
        BUILD_CMD="docker buildx build --platform ${PLATFORMS} -t ${FULL_IMAGE_NAME} -f ${DOCKERFILE} --load"
    fi
else
    echo "Note: Images will be built but not pushed or loaded."
    echo "Use --push to push to registry or --load to load locally."
fi

BUILD_CMD+=" ."

echo ""
echo "Executing: $BUILD_CMD"
echo ""

# Change to repository root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

# Execute the build
eval "$BUILD_CMD"

if [ $? -eq 0 ]; then
    echo ""
    echo "======================================"
    echo "Build completed successfully!"
    echo "======================================"
    echo "Image: ${FULL_IMAGE_NAME}"
    echo "Platforms: ${PLATFORMS}"
    
    if [ "$PUSH" = true ]; then
        echo ""
        echo "Images have been pushed to the registry."
        echo "You can pull them with:"
        echo "  docker pull ${FULL_IMAGE_NAME}"
    elif [ "$LOAD" = true ]; then
        echo ""
        echo "Image has been loaded into local Docker."
        
        if [ "$RUN" = true ]; then
            echo ""
            echo "Starting container with workspace mounted at /opt/datacrumbs..."
            docker run -ti --privileged --cap-add sys_admin --cap-add sys_ptrace \
                --net=host --pid=host --hostname docker \
                -v /lib/modules/:/lib/modules:ro \
                -v /sys/kernel/debug/:/sys/kernel/debug:rw \
                -v /sys/fs/bpf:/sys/fs/bpf \
                -v "$(pwd):/opt/datacrumbs" -w /opt/datacrumbs \
                ${FULL_IMAGE_NAME}
        else
            echo ""
            echo "To run with current workspace mounted (replaces built version in container):"
            echo "  docker run -ti --privileged --cap-add sys_admin --cap-add sys_ptrace \\"
            echo "    --net=host --pid=host --hostname docker \\"
            echo "    -v /lib/modules/:/lib/modules:ro \\"
            echo "    -v /sys/kernel/debug/:/sys/kernel/debug:rw \\"
            echo "    -v /sys/fs/bpf:/sys/fs/bpf \\"
            echo "    -v \"\$(pwd):/opt/datacrumbs\" -w /opt/datacrumbs \\"
            echo "    ${FULL_IMAGE_NAME}"
            echo ""
            echo "To run with workspace mounted at /workspace (keeps built version):"
            echo "  docker run -ti --privileged --cap-add sys_admin --cap-add sys_ptrace \\"
            echo "    --net=host --pid=host --hostname docker \\"
            echo "    -v /lib/modules/:/lib/modules:ro \\"
            echo "    -v /sys/kernel/debug/:/sys/kernel/debug:rw \\"
            echo "    -v /sys/fs/bpf:/sys/fs/bpf \\"
            echo "    -v \"\$(pwd):/workspace\" -w /workspace \\"
            echo "    ${FULL_IMAGE_NAME}"
        fi
    fi
else
    echo ""
    echo "======================================"
    echo "Build failed!"
    echo "======================================"
    exit 1
fi
