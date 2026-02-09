//=============================================================================
// LadybugExport - Command-line tool for exporting Ladybug camera images
// 
// This tool works like ladybugProcessStream.exe with additional options:
// - Export individual camera images (-x 6processed)
// - Export panorama with rotation angle (-q "Front X -Down Y")
//
// Based on the official Ladybug SDK ladybugProcessStream sample.
//
// Usage (compatible with ladybugProcessStream.exe):
//   LadybugExport.exe -i stream.pgr -o output_prefix [OPTIONS]
//
// Platform: Windows x64
//=============================================================================

#include <windows.h>
#include <direct.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <regex>

//=============================================================================
// Ladybug SDK headers
//=============================================================================
#include <ladybug.h>
#include <ladybugrenderer.h>
#include <ladybuggeom.h>
#include <ladybugstream.h>

//=============================================================================
// Constants
//=============================================================================

// Pi constant for angle conversions
constexpr double PI = 3.14159265358979323846;

// Default panorama dimensions
constexpr int DEFAULT_PANO_WIDTH = 2048;
constexpr int DEFAULT_PANO_HEIGHT = 1024;

//=============================================================================
// Command-Line Arguments Structure (matches ladybugProcessStream.exe)
//=============================================================================

struct CommandLineArgs
{
    std::string inputFile;              // -i Input .pgr stream file
    std::string outputPrefix;           // -o Output path (folder + file prefix)
    std::string frameRange;             // -r Frame range "start-end"
    unsigned int startFrame = 0;
    unsigned int endFrame = 0;
    bool processAllFrames = true;
    
    int panoWidth = DEFAULT_PANO_WIDTH;     // -w Output width
    int panoHeight = DEFAULT_PANO_HEIGHT;   // -w Output height
    
    std::string renderType = "pano";        // -t Render type (pano, 6processed, etc.)
    std::string format = "jpg";             // -f Output format
    std::string colorProcessing = "hq";     // -c Color processing method
    
    int blendingWidth = 100;                // -b Blending width
    float falloffValue = 1.0f;              // -v Falloff correction value
    bool falloffEnabled = false;            // -a Enable falloff correction
    bool softwareRendering = false;         // -s Software rendering
    bool antiAliasing = false;              // -k Anti-aliasing
    bool stabilization = false;             // -z Stabilization
    
    // NEW OPTIONS:
    // -x 6processed  : Export 6 individual camera images
    std::string exportType;                 // -x Export type (6processed)
    bool export6Cameras = false;            // Flag for 6 camera export
    
    // -q "Front X -Down Y" : Rotation angle for panorama
    std::string rotationAngle;              // -q Rotation angle string
    double rotFront = 0.0;                  // Pitch (Front rotation)
    double rotDown = 0.0;                   // Yaw (Down rotation)
    
    // Standard rotation (from original ladybugProcessStream)
    float fov = 60.0f;                      // -q FOV for spherical
    float rotX = 0.0f;                      // -x Euler rotation X
    float rotY = 0.0f;                      // Euler rotation Y
    float rotZ = 0.0f;                      // Euler rotation Z
};

//=============================================================================
// Global Variables (following SDK sample pattern)
//=============================================================================

LadybugContext context = nullptr;
LadybugStreamContext streamContext = nullptr;
LadybugStreamHeadInfo streamHeaderInfo;
LadybugImage image;
unsigned int textureWidth = 0;
unsigned int textureHeight = 0;
unsigned char* textureBuffers[LADYBUG_NUM_CAMERAS] = {nullptr};
bool isHighBitDepth = false;  // True for 12/16-bit formats
char tempConfigPath[MAX_PATH] = {0};

//=============================================================================
// Helper Macro for Error Checking
//=============================================================================

#define CHECK_ERROR(error, functionName) \
    if (error != LADYBUG_OK) { \
        printf("Error [%s]: %s\n", functionName, ladybugErrorToString(error)); \
        return error; \
    }

//=============================================================================
// Helper Functions
//=============================================================================

/**
 * @brief Check if data format is high bit depth (12/16 bit)
 */
bool IsHighBitDepthFormat(LadybugDataFormat format)
{
    return (
        format == LADYBUG_DATAFORMAT_RAW12 ||
        format == LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW12 ||
        format == LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12 ||
        format == LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12 ||
        format == LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12_PROCESSED ||
        format == LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12_PROCESSED ||
        format == LADYBUG_DATAFORMAT_RAW16 ||
        format == LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW16);
}

/**
 * @brief Case-insensitive string comparison
 */
int strncmpCaseInsensitive(const char* s1, const char* s2, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        int c1 = tolower((unsigned char)s1[i]);
        int c2 = tolower((unsigned char)s2[i]);
        if (c1 != c2) return c1 - c2;
        if (c1 == '\0') return 0;
    }
    return 0;
}

/**
 * @brief Prints usage information (compatible with ladybugProcessStream.exe style)
 */
void PrintUsage(const char* programName)
{
    printf("\nUsage:\n\n");
    printf("%s [OPTIONS]\n\n", programName);
    printf("OPTIONS\n\n");
    printf("  -i STREAM_PATH     The PGR stream file to process with an extension of .pgr\n");
    printf("  -r NNN-NNN         The frame range to process. The first frame is 0.\n");
    printf("                     Default setting is to process all the images.\n");
    printf("  -o OUTPUT_PATH     Output file prefix.\n");
    printf("                     Default is ladybugImageOutput\n");
    printf("  -w NNNNxNNNN       Output image size (widthxheight) in pixel.\n");
    printf("                     Default is 2048x1024.\n");
    printf("  -t RENDER_TYPE     Output image rendering type:\n");
    printf("              pano      - panoramic view (default)\n");
    printf("              dome      - dome view\n");
    printf("              spherical - spherical view\n");
    printf("              rectify-0 - rectified image (camera 0)\n");
    printf("              rectify-1 - rectified image (camera 1)\n");
    printf("              rectify-2 - rectified image (camera 2)\n");
    printf("              rectify-3 - rectified image (camera 3)\n");
    printf("              rectify-4 - rectified image (camera 4)\n");
    printf("              rectify-5 - rectified image (camera 5)\n");
    printf("  -f FORMAT          Output image format:\n");
    printf("              bmp      - Windows BMP image\n");
    printf("              jpg      - JPEG image (default)\n");
    printf("              tiff     - TIFF image\n");
    printf("              png      - PNG image\n");
    printf("  -c COLOR_PROCESS   Debayering method:\n");
    printf("              hq       - High quality linear method (default)\n");
    printf("              hq-gpu   - High quality linear method (GPU)\n");
    printf("              edge     - Edge sensing method\n");
    printf("              near     - Nearest neighbor method\n");
    printf("              near-f   - Nearest neighbor(fast) method\n");
    printf("              down4    - Downsample4 method\n");
    printf("              down16   - Downsample16 method\n");
    printf("              mono     - Monochrome method\n");
    printf("  -b NNN             Blending width in pixel. Default is 100.\n");
    printf("  -s true/false      Enable software rendering. Default is false.\n");
    printf("  -k true/false      Enable anti-aliasing. Default is false.\n");
    printf("\n");
    printf("NEW OPTIONS (extended functionality):\n\n");
    printf("  -x EXPORT_TYPE     Export type for individual cameras:\n");
    printf("              6processed - Export all 6 processed camera images\n");
    printf("\n");
    printf("  -q ROTATION        Rotation angle for panorama orientation.\n");
    printf("                     Format: \"Front X -Down Y\" where X and Y are degrees.\n");
    printf("                     Front = pitch rotation (positive = look up)\n");
    printf("                     Down = yaw rotation (positive = rotate right)\n");
    printf("                     Example: -q \"Front 5 -Down 0\"\n");
    printf("                              -q \"Front -10 -Down 45\"\n");
    printf("\n");
    printf("EXAMPLES\n\n");
    printf("  %s -i stream.pgr -o output -t pano -f jpg -c hq\n\n", programName);
    printf("        Process stream and export panoramic JPG images.\n\n");
    printf("  %s -i stream.pgr -o output -x 6processed -f jpg -c hq\n\n", programName);
    printf("        Export all 6 processed camera images as JPG.\n\n");
    printf("  %s -i stream.pgr -o output -t pano -q \"Front 5 -Down 0\" -f jpg\n\n", programName);
    printf("        Export panorama with Front=5 degrees pitch rotation.\n\n");
}

/**
 * @brief Parses the rotation angle string "Front X -Down Y"
 */
bool ParseRotationAngle(const std::string& rotStr, double& front, double& down)
{
    // Parse format: "Front X -Down Y" or variations
    // Examples: "Front 5 -Down 0", "Front -10 -Down 45", "Front 0 -Down -90"
    
    front = 0.0;
    down = 0.0;
    
    // Try to find "Front" value
    std::regex frontRegex(R"(Front\s+(-?\d+\.?\d*))", std::regex::icase);
    std::smatch frontMatch;
    if (std::regex_search(rotStr, frontMatch, frontRegex))
    {
        front = std::stod(frontMatch[1].str());
    }
    
    // Try to find "Down" value (may be preceded by '-')
    std::regex downRegex(R"(-?Down\s+(-?\d+\.?\d*))", std::regex::icase);
    std::smatch downMatch;
    if (std::regex_search(rotStr, downMatch, downRegex))
    {
        down = std::stod(downMatch[1].str());
    }
    
    return true;
}

/**
 * @brief Parses frame range string "start-end"
 */
bool ParseFrameRange(const std::string& rangeStr, unsigned int& start, unsigned int& end)
{
    size_t dashPos = rangeStr.find('-');
    if (dashPos == std::string::npos)
    {
        return false;
    }
    
    try
    {
        start = static_cast<unsigned int>(std::stoul(rangeStr.substr(0, dashPos)));
        end = static_cast<unsigned int>(std::stoul(rangeStr.substr(dashPos + 1)));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

/**
 * @brief Parses resolution string "WIDTHxHEIGHT"
 */
bool ParseResolution(const std::string& resStr, int& width, int& height)
{
    size_t xPos = resStr.find('x');
    if (xPos == std::string::npos)
    {
        xPos = resStr.find('X');
    }
    if (xPos == std::string::npos)
    {
        return false;
    }
    
    try
    {
        width = std::stoi(resStr.substr(0, xPos));
        height = std::stoi(resStr.substr(xPos + 1));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

/**
 * @brief Parses command-line arguments (ladybugProcessStream.exe compatible)
 */
bool ParseCommandLine(int argc, char* argv[], CommandLineArgs& args)
{
    if (argc < 2)
    {
        printf("Error: No arguments provided.\n");
        PrintUsage(argv[0]);
        return false;
    }

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        
        // Check for help
        if (arg == "-?" || arg == "-h" || arg == "--help")
        {
            PrintUsage(argv[0]);
            return false;
        }
        
        // All options require a following parameter
        if (arg.length() >= 2 && arg[0] == '-' && i + 1 < argc)
        {
            const char* param = argv[++i];
            
            if (arg == "-i")
            {
                args.inputFile = param;
            }
            else if (arg == "-o")
            {
                args.outputPrefix = param;
            }
            else if (arg == "-r")
            {
                args.frameRange = param;
                if (ParseFrameRange(param, args.startFrame, args.endFrame))
                {
                    args.processAllFrames = false;
                }
                else
                {
                    printf("Warning: Invalid frame range '%s'. Processing all frames.\n", param);
                }
            }
            else if (arg == "-w")
            {
                if (!ParseResolution(param, args.panoWidth, args.panoHeight))
                {
                    printf("Warning: Invalid resolution '%s'. Using default.\n", param);
                }
            }
            else if (arg == "-t")
            {
                args.renderType = param;
            }
            else if (arg == "-f")
            {
                args.format = param;
            }
            else if (arg == "-c")
            {
                args.colorProcessing = param;
            }
            else if (arg == "-b")
            {
                args.blendingWidth = std::stoi(param);
            }
            else if (arg == "-s")
            {
                args.softwareRendering = (strncmpCaseInsensitive(param, "true", 4) == 0);
            }
            else if (arg == "-k")
            {
                args.antiAliasing = (strncmpCaseInsensitive(param, "true", 4) == 0);
            }
            else if (arg == "-v")
            {
                args.falloffValue = std::stof(param);
            }
            else if (arg == "-a")
            {
                args.falloffEnabled = (strncmpCaseInsensitive(param, "true", 4) == 0);
            }
            else if (arg == "-z")
            {
                args.stabilization = (strncmpCaseInsensitive(param, "true", 4) == 0);
            }
            // NEW OPTIONS:
            else if (arg == "-x")
            {
                args.exportType = param;
                if (strncmpCaseInsensitive(param, "6processed", 10) == 0)
                {
                    args.export6Cameras = true;
                }
                else
                {
                    printf("Warning: Unknown export type '%s'. Use '6processed'.\n", param);
                }
            }
            else if (arg == "-q")
            {
                args.rotationAngle = param;
                ParseRotationAngle(param, args.rotFront, args.rotDown);
            }
            else
            {
                printf("Warning: Unknown option '%s' ignored.\n", arg.c_str());
                --i; // Didn't consume param
            }
        }
        else
        {
            printf("Warning: Unknown argument '%s' ignored.\n", arg.c_str());
        }
    }

    // Validate required arguments
    if (args.inputFile.empty())
    {
        printf("Error: Input file not specified. Use -i <file.pgr>\n");
        return false;
    }

    if (args.outputPrefix.empty())
    {
        args.outputPrefix = "ladybugImageOutput";
    }

    return true;
}

//=============================================================================
// Helper Functions - File Format
//=============================================================================

LadybugSaveFileFormat GetSaveFormat(const std::string& format)
{
    if (format == "bmp")  return LADYBUG_FILEFORMAT_BMP;
    if (format == "jpg")  return LADYBUG_FILEFORMAT_JPG;
    if (format == "jpeg") return LADYBUG_FILEFORMAT_JPG;
    if (format == "tiff") return LADYBUG_FILEFORMAT_TIFF;
    if (format == "png")  return LADYBUG_FILEFORMAT_PNG;
    return LADYBUG_FILEFORMAT_JPG; // Default to JPG like ladybugProcessStream
}

const char* GetFileExtension(const std::string& format)
{
    if (format == "bmp")  return "bmp";
    if (format == "jpg")  return "jpg";
    if (format == "jpeg") return "jpg";
    if (format == "tiff") return "tiff";
    if (format == "png")  return "png";
    return "jpg";
}

LadybugColorProcessingMethod GetColorProcessingMethod(const std::string& method)
{
    if (method == "hq" || method == "hq-gpu") return LADYBUG_HQLINEAR;
    if (method == "edge") return LADYBUG_EDGE_SENSING;
    if (method == "near") return LADYBUG_NEAREST_NEIGHBOR_FAST;
    if (method == "near-f") return LADYBUG_NEAREST_NEIGHBOR_FAST;
    if (method == "down4") return LADYBUG_DOWNSAMPLE4;
    if (method == "down16") return LADYBUG_DOWNSAMPLE16;
    if (method == "mono") return LADYBUG_MONO;
    return LADYBUG_HQLINEAR;
}

//=============================================================================
// Helper Functions - Directory Creation
//=============================================================================

bool CreateDirectoryRecursive(const std::string& path)
{
    size_t pos = 0;
    std::string currentPath;
    
    while ((pos = path.find_first_of("\\/", pos + 1)) != std::string::npos)
    {
        currentPath = path.substr(0, pos);
        if (!currentPath.empty())
        {
            _mkdir(currentPath.c_str());
        }
    }
    
    if (_mkdir(path.c_str()) == 0 || errno == EEXIST)
    {
        return true;
    }
    
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::string GetDirectoryFromPath(const std::string& path)
{
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos)
    {
        return path.substr(0, pos);
    }
    return ".";
}

//=============================================================================
// Ladybug SDK Initialization
//=============================================================================

LadybugError InitializeLadybug(const CommandLineArgs& args)
{
    LadybugError error;

    printf("Initializing Ladybug SDK...\n");

    // Create contexts
    error = ladybugCreateContext(&context);
    CHECK_ERROR(error, "ladybugCreateContext");

    error = ladybugCreateStreamContext(&streamContext);
    CHECK_ERROR(error, "ladybugCreateStreamContext");

    // Open stream file
    printf("Opening stream file: %s\n", args.inputFile.c_str());
    error = ladybugInitializeStreamForReading(streamContext, args.inputFile.c_str(), true);
    CHECK_ERROR(error, "ladybugInitializeStreamForReading");

    // Extract config file
    char* tempFile = _tempnam(NULL, "lb_cfg_");
    if (tempFile != NULL)
    {
        strncpy_s(tempConfigPath, tempFile, MAX_PATH);
        free(tempFile);
        
        error = ladybugGetStreamConfigFile(streamContext, tempConfigPath);
        if (error != LADYBUG_OK)
        {
            printf("Warning: Could not extract config file: %s\n", ladybugErrorToString(error));
            tempConfigPath[0] = '\0';
        }
    }

    // Load configuration
    if (strlen(tempConfigPath) > 0)
    {
        error = ladybugLoadConfig(context, tempConfigPath);
        CHECK_ERROR(error, "ladybugLoadConfig");
        remove(tempConfigPath);
    }

    // Get stream header
    error = ladybugGetStreamHeader(streamContext, &streamHeaderInfo);
    CHECK_ERROR(error, "ladybugGetStreamHeader");

    float frameRate = (streamHeaderInfo.ulLadybugStreamVersion < 7) 
        ? (float)streamHeaderInfo.ulFrameRate 
        : streamHeaderInfo.frameRate;

    printf("\n--- Stream Information ---\n");
    printf("Base S/N: %d\n", streamHeaderInfo.serialBase);
    printf("Head S/N: %d\n", streamHeaderInfo.serialHead);
    printf("Frame rate : %.2f\n", frameRate);
    printf("Data format: %d\n", streamHeaderInfo.dataFormat);
    printf("Resolution: %d\n", streamHeaderInfo.resolution);
    printf("Stream version: %lu\n", streamHeaderInfo.ulLadybugStreamVersion);
    printf("--------------------------\n\n");

    // Check if this is a high bit depth format
    isHighBitDepth = IsHighBitDepthFormat(streamHeaderInfo.dataFormat);
    if (isHighBitDepth)
    {
        printf("Detected high bit depth format (12/16-bit)\n");
    }

    // Set color processing method
    printf("Setting debayering method...\n");
    LadybugColorProcessingMethod colorMethod = GetColorProcessingMethod(args.colorProcessing);
    error = ladybugSetColorProcessingMethod(context, colorMethod);
    CHECK_ERROR(error, "ladybugSetColorProcessingMethod");

    // Read one image to get dimensions
    error = ladybugReadImageFromStream(streamContext, &image);
    CHECK_ERROR(error, "ladybugReadImageFromStream (initial)");

    printf("Image info: %dx%d, format=%d\n", 
           image.uiCols, image.uiRows, image.dataFormat);

    // Set texture dimensions based on color processing method
    if (colorMethod == LADYBUG_DOWNSAMPLE4)
    {
        textureWidth = image.uiCols / 2;
        textureHeight = image.uiRows / 2;
    }
    else if (colorMethod == LADYBUG_DOWNSAMPLE16)
    {
        textureWidth = image.uiCols / 4;
        textureHeight = image.uiRows / 4;
    }
    else
    {
        textureWidth = image.uiCols;
        textureHeight = image.uiRows;
    }

    // Allocate texture buffers (2x size for 16-bit formats)
    const unsigned int bytesPerPixel = isHighBitDepth ? 8 : 4;  // BGRU16 vs BGRU
    for (int i = 0; i < LADYBUG_NUM_CAMERAS; i++)
    {
        textureBuffers[i] = new unsigned char[textureWidth * textureHeight * bytesPerPixel];
        if (textureBuffers[i] == nullptr)
        {
            printf("Error: Failed to allocate texture buffer for camera %d\n", i);
            return LADYBUG_MEMORY_ALLOC_ERROR;
        }
    }

    // Set blending width
    error = ladybugSetBlendingParams(context, args.blendingWidth);
    if (error != LADYBUG_OK)
    {
        printf("Warning: Could not set blending params: %s\n", ladybugErrorToString(error));
    }

    // Initialize alpha masks
    printf("Initializing alpha masks (this may take some time)...\n");
    error = ladybugInitializeAlphaMasks(context, textureWidth, textureHeight);
    if (error != LADYBUG_OK)
    {
        printf("Warning: Could not initialize alpha masks: %s\n", ladybugErrorToString(error));
    }

    error = ladybugSetAlphaMasking(context, true);
    if (error != LADYBUG_OK)
    {
        printf("Warning: Could not enable alpha masking: %s\n", ladybugErrorToString(error));
    }

    // Configure panoramic output (for pano export)
    if (!args.export6Cameras)
    {
        printf("Configure output images in Ladybug library...\n");
        error = ladybugConfigureOutputImages(context, LADYBUG_PANORAMIC);
        CHECK_ERROR(error, "ladybugConfigureOutputImages");

        printf("Set off-screen panoramic image size:%dx%d image.\n", args.panoWidth, args.panoHeight);
        error = ladybugSetOffScreenImageSize(context, LADYBUG_PANORAMIC, args.panoWidth, args.panoHeight);
        CHECK_ERROR(error, "ladybugSetOffScreenImageSize");

        // Apply rotation for panoramic images using ladybugSet3dMapRotation
        // This rotates the 3D mesh used for panorama stitching
        // Front = pitch (rotation around X axis)
        // Down = yaw (rotation around Y axis)
        if (args.rotFront != 0.0 || args.rotDown != 0.0)
        {
            double rotX = args.rotFront * PI / 180.0;  // Pitch (Front) in radians
            double rotY = args.rotDown * PI / 180.0;   // Yaw (Down) in radians
            double rotZ = 0.0;  // Roll

            error = ladybugSet3dMapRotation(context, rotX, rotY, rotZ);
            if (error != LADYBUG_OK)
            {
                printf("Warning: Could not set 3D map rotation: %s\n", ladybugErrorToString(error));
            }
            else
            {
                printf("Applied rotation: Front=%.1f, Down=%.1f degrees\n", args.rotFront, args.rotDown);
            }
        }
    }

    // Rewind stream
    error = ladybugGoToImage(streamContext, 0);
    CHECK_ERROR(error, "ladybugGoToImage (rewind)");

    return LADYBUG_OK;
}

//=============================================================================
// Cleanup
//=============================================================================

void CleanupLadybug()
{
    for (int i = 0; i < LADYBUG_NUM_CAMERAS; i++)
    {
        if (textureBuffers[i] != nullptr)
        {
            delete[] textureBuffers[i];
            textureBuffers[i] = nullptr;
        }
    }

    if (streamContext != nullptr)
    {
        ladybugDestroyStreamContext(&streamContext);
        streamContext = nullptr;
    }

    if (context != nullptr)
    {
        ladybugDestroyContext(&context);
        context = nullptr;
    }

    if (strlen(tempConfigPath) > 0)
    {
        remove(tempConfigPath);
    }
}

//=============================================================================
// Export Functions
//=============================================================================

/**
 * @brief Export 6 processed camera images for a single frame
 */
LadybugError Export6CameraImages(unsigned int frameNum, const CommandLineArgs& args)
{
    LadybugError error;
    LadybugSaveFileFormat saveFormat = GetSaveFormat(args.format);
    const char* ext = GetFileExtension(args.format);

    for (int cam = 0; cam < LADYBUG_NUM_CAMERAS; cam++)
    {
        LadybugProcessedImage processedImage;
        memset(&processedImage, 0, sizeof(processedImage));
        processedImage.pData = textureBuffers[cam];
        processedImage.uiCols = textureWidth;
        processedImage.uiRows = textureHeight;
        processedImage.pixelFormat = isHighBitDepth ? LADYBUG_BGRU16 : LADYBUG_BGRU;

        // Generate filename: outputDir\frameNum_camNum.ext
        char filename[MAX_PATH];
        sprintf_s(filename, "%s\\%06u_cam%d.%s", 
                  args.outputPrefix.c_str(), frameNum, cam, ext);

        error = ladybugSaveImage(context, &processedImage, filename, saveFormat, false);
        if (error != LADYBUG_OK)
        {
            printf("Warning: Could not save camera %d image: %s\n", cam, ladybugErrorToString(error));
        }
    }

    return LADYBUG_OK;
}

/**
 * @brief Export panoramic image for a single frame
 */
LadybugError ExportPanorama(unsigned int frameNum, const CommandLineArgs& args)
{
    LadybugError error;
    LadybugSaveFileFormat saveFormat = GetSaveFormat(args.format);
    const char* ext = GetFileExtension(args.format);

    LadybugProcessedImage processedImage;
    error = ladybugRenderOffScreenImage(context, LADYBUG_PANORAMIC, LADYBUG_BGR, &processedImage);
    if (error != LADYBUG_OK)
    {
        printf("Error: Could not render panorama: %s\n", ladybugErrorToString(error));
        return error;
    }

    // Generate filename: outputDir\frameNum.ext
    char filename[MAX_PATH];
    sprintf_s(filename, "%s\\%06u.%s", args.outputPrefix.c_str(), frameNum, ext);

    error = ladybugSaveImage(context, &processedImage, filename, saveFormat, false);
    if (error != LADYBUG_OK)
    {
        printf("Error: Could not save panorama: %s\n", ladybugErrorToString(error));
        return error;
    }

    printf("Getting panoramic image and writing it to %s...\n", filename);
    return LADYBUG_OK;
}

//=============================================================================
// Main Processing Function
//=============================================================================

int ProcessStream(const CommandLineArgs& args)
{
    LadybugError error;

    // Get total frames
    unsigned int totalFrames = 0;
    error = ladybugGetStreamNumOfImages(streamContext, &totalFrames);
    if (error != LADYBUG_OK)
    {
        printf("Error: Could not get frame count: %s\n", ladybugErrorToString(error));
        return -1;
    }

    // Determine frame range
    unsigned int startFrame = args.processAllFrames ? 0 : args.startFrame;
    unsigned int endFrame = args.processAllFrames ? totalFrames - 1 : args.endFrame;
    
    if (endFrame >= totalFrames)
    {
        endFrame = totalFrames - 1;
    }

    // Create output directory (-o is treated as the output folder)
    CreateDirectoryRecursive(args.outputPrefix);

    // Go to start frame
    if (startFrame > 0)
    {
        error = ladybugGoToImage(streamContext, startFrame);
        if (error != LADYBUG_OK)
        {
            printf("Error: Could not seek to frame %u: %s\n", startFrame, ladybugErrorToString(error));
            return -1;
        }
    }

    // Process frames
    for (unsigned int frame = startFrame; frame <= endFrame; frame++)
    {
        printf("Processing frame %u of %u\n", frame, endFrame);

        // Read frame
        error = ladybugReadImageFromStream(streamContext, &image);
        if (error != LADYBUG_OK)
        {
            printf("Warning: Could not read frame %u: %s\n", frame, ladybugErrorToString(error));
            continue;
        }

        // Convert image - use BGRU16 for high bit depth, BGRU for 8-bit
        LadybugPixelFormat pixelFormat = isHighBitDepth ? LADYBUG_BGRU16 : LADYBUG_BGRU;
        error = ladybugConvertImage(context, &image, textureBuffers, pixelFormat);
        if (error != LADYBUG_OK)
        {
            printf("Warning: Could not convert frame %u: %s\n", frame, ladybugErrorToString(error));
            continue;
        }

        if (args.export6Cameras)
        {
            // Export 6 camera images
            Export6CameraImages(frame, args);
        }
        else
        {
            // Update textures for panorama
            error = ladybugUpdateTextures(context, LADYBUG_NUM_CAMERAS, 
                                          (const unsigned char**)textureBuffers, pixelFormat);
            if (error != LADYBUG_OK)
            {
                printf("Warning: Could not update textures for frame %u: %s\n", frame, ladybugErrorToString(error));
                continue;
            }

            // Export panorama
            ExportPanorama(frame, args);
        }
    }

    return 0;
}

//=============================================================================
// Main Entry Point
//=============================================================================

int main(int argc, char* argv[])
{
    CommandLineArgs args;

    // Parse command line
    if (!ParseCommandLine(argc, argv, args))
    {
        return 1;
    }

    // Print configuration
    printf("\n");
    if (args.export6Cameras)
    {
        printf("Export type: 6 Processed Camera Images\n");
    }
    else
    {
        printf("Export type: Panoramic (%dx%d)\n", args.panoWidth, args.panoHeight);
        if (args.rotFront != 0.0 || args.rotDown != 0.0)
        {
            printf("Rotation: Front %.1f, Down %.1f degrees\n", args.rotFront, args.rotDown);
        }
    }
    printf("Output format: %s\n", args.format.c_str());
    printf("Color processing: %s\n", args.colorProcessing.c_str());
    printf("\n");

    // Initialize SDK
    if (InitializeLadybug(args) != LADYBUG_OK)
    {
        printf("Failed to initialize Ladybug SDK.\n");
        CleanupLadybug();
        return 1;
    }

    // Process stream
    int result = ProcessStream(args);

    // Cleanup
    CleanupLadybug();

    if (result == 0)
    {
        printf("\nExport complete.\n");
    }

    return result;
}
