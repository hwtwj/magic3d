#include "VideoRecordingApp.h"
#include "../Common/LogSystem.h"
#include "../Common/RenderSystem.h"
#include "../Common/ToolKit.h"

namespace MagicApp
{
    VideoRecordingApp::VideoRecordingApp() :
        mpVideoCanvas(NULL),
        mpVCTex(NULL),
        mpVCMat(NULL),
        mTexWidth(1024),
        mTexHeight(1024),
        mVideoCapture(),
        mIsUpdateVideoCanvas(false),
        mVideoWidth(1),
        mVideoHeight(1),
        mOneFrameTime(0),
        mTimeAccumulate(0),
        mIsRecording(false)
    {
    }

    VideoRecordingApp::~VideoRecordingApp()
    {
    }

    bool VideoRecordingApp::Enter(void)
    {
        InfoLog << "Enter VideoRecordingApp" << std::endl; 
        mUI.Setup();
        SetupScene();
        
        return true;
    }

    bool VideoRecordingApp::Update(float timeElapsed)
    {
        return true;
    }

    bool VideoRecordingApp::Exit(void)
    {
        ShutdownScene();
        mUI.Shutdown();
        return true;
    }

    void VideoRecordingApp::WindowResized( Ogre::RenderWindow* rw )
    {

    }

    void VideoRecordingApp::SetupScene(void)
    {
        InfoLog << "VideoRecordingApp::SetupScene" << std::endl;
        Ogre::SceneManager* pSceneMgr = MagicCore::RenderSystem::GetSingleton()->GetSceneManager();
        pSceneMgr->setAmbientLight(Ogre::ColourValue(0.1, 0.1, 0.1));
        Ogre::Light* sl = pSceneMgr->createLight("SimpleLight");
        sl->setPosition(0, 0, 20);
        sl->setDiffuseColour(0.8, 0.8, 0.8);
        sl->setSpecularColour(0.5, 0.5, 0.5);

        //Setup Video Canvas
        mpVCTex = Ogre::TextureManager::getSingleton().createManual(
            "VideoRecordingTex",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            mTexWidth,
            mTexHeight,
            0,
            Ogre::PF_B8G8R8A8,
            Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE
            );
        Ogre::HardwarePixelBufferSharedPtr pVCPixelBuffer = mpVCTex->getBuffer();
        unsigned char* pVCBuffer = static_cast<unsigned char*>(
            pVCPixelBuffer->lock(0, mTexWidth * mTexHeight * 4, Ogre::HardwareBuffer::HBL_DISCARD) );
        for (int yid = 0; yid < mTexHeight; yid++)
        {
            for (int xid = 0; xid < mTexWidth; xid++)
            {
                pVCBuffer[ (yid * mTexWidth + xid) * 4 + 0] = 220;
                pVCBuffer[ (yid * mTexWidth + xid) * 4 + 1] = 220;
                pVCBuffer[ (yid * mTexWidth + xid) * 4 + 2] = 220;
                pVCBuffer[ (yid * mTexWidth + xid) * 4 + 3] = 255;
            }
        }
        pVCPixelBuffer->unlock();

        mpVCMat = Ogre::MaterialManager::getSingleton().create(
            "VideoRecordingMat",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mpVCMat->getTechnique(0)->getPass(0)->createTextureUnitState("VideoRecordingTex");
        mpVCMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);

        mpVideoCanvas = new Ogre::Rectangle2D(true);
        mpVideoCanvas->setBoundingBox(Ogre::AxisAlignedBox(-100000.0f * Ogre::Vector3::UNIT_SCALE, 100000.0f * Ogre::Vector3::UNIT_SCALE));
        mpVideoCanvas->setMaterial("VideoRecordingMat");
        Ogre::RenderWindow* rw = MagicCore::RenderSystem::GetSingleton()->GetRenderWindow();
        int winW = rw->getWidth();
        int winH = rw->getHeight();
        UpdateCanvasSize(winW, winH, mVideoWidth, mVideoHeight);

        if (pSceneMgr->hasSceneNode("ModelNode"))
        {
            pSceneMgr->getSceneNode("ModelNode")->attachObject(mpVideoCanvas);
        }
        else
        {
            pSceneMgr->getRootSceneNode()->createChildSceneNode("ModelNode")->attachObject(mpVideoCanvas);
        }
    }

    void VideoRecordingApp::ShutdownScene(void)
    {

    }

    void VideoRecordingApp::UpdateCanvas(float timeElapsed)
    {
        cv::Mat newFrameOrigin;
        mVideoCapture >> newFrameOrigin;
        /*if (mVideoCapture.read(newFrameOrigin) == false)
        {
            DebugLog << "Video Begin Again" << std::endl;
            mVideoCapture.set(CV_CAP_PROP_POS_FRAMES, 0);
            mVideoCapture.read(newFrameOrigin);
        }*/
        cv::Size vcSize(mTexWidth, mTexHeight);
        cv::Mat newFrame(vcSize, CV_8UC3);
        cv::resize(newFrameOrigin, newFrame, vcSize);
        int nChannel = newFrame.channels();
        //DebugLog << "nChannel: " << nChannel << std::endl;
        if (nChannel == 1)
        {
            Ogre::HardwarePixelBufferSharedPtr pVCPixelBuffer = mpVCTex->getBuffer();
            unsigned char* pVCBuffer = static_cast<unsigned char*>(
                pVCPixelBuffer->lock(0, mTexWidth * mTexHeight * 4, Ogre::HardwareBuffer::HBL_DISCARD) );
            for (int yid = 0; yid < mTexHeight; yid++)
            {
                for (int xid = 0; xid < mTexWidth; xid++)
                {
                    unsigned char* pixel = newFrame.ptr(xid, yid);
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 0] = pixel[0];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 1] = pixel[0];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 2] = pixel[0];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 3] = 255;
                }
            }
            pVCPixelBuffer->unlock();
        }
        else if (nChannel == 3)
        {
            Ogre::HardwarePixelBufferSharedPtr pVCPixelBuffer = mpVCTex->getBuffer();
            unsigned char* pVCBuffer = static_cast<unsigned char*>(
                pVCPixelBuffer->lock(0, mTexWidth * mTexHeight * 4, Ogre::HardwareBuffer::HBL_DISCARD) );
            for (int yid = 0; yid < mTexHeight; yid++)
            {
                for (int xid = 0; xid < mTexWidth; xid++)
                {
                    unsigned char* pixel = newFrame.ptr(yid, xid);
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 0] = pixel[0];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 1] = pixel[1];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 2] = pixel[2];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 3] = 255;
                }
            }
            pVCPixelBuffer->unlock();
        }
        else if (nChannel >= 4)
        {
            Ogre::HardwarePixelBufferSharedPtr pVCPixelBuffer = mpVCTex->getBuffer();
            unsigned char* pVCBuffer = static_cast<unsigned char*>(
                pVCPixelBuffer->lock(0, mTexWidth * mTexHeight * 4, Ogre::HardwareBuffer::HBL_DISCARD) );
            for (int yid = 0; yid < mTexHeight; yid++)
            {
                for (int xid = 0; xid < mTexWidth; xid++)
                {
                    unsigned char* pixel = newFrame.ptr(xid, yid);
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 0] = pixel[0];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 1] = pixel[1];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 2] = pixel[2];
                    pVCBuffer[ (yid * mTexWidth + xid) * 4 + 3] = pixel[3];
                }
            }
            pVCPixelBuffer->unlock();
        }
        if (mIsRecording == true)
        {
            mTimeAccumulate += timeElapsed;
            if (mTimeAccumulate > mOneFrameTime)
            {
                mVideoWriter << newFrameOrigin;
                mTimeAccumulate -= mOneFrameTime;
            }
        }
    }

    void VideoRecordingApp::UpdateCanvasSize(int winW, int winH, int videoW, int videoH)
    {
        float canvasW, canvasH;
        float maxSize = 0.8;
        if (videoW > videoH)
        {
            canvasW = maxSize;
            canvasH = maxSize * videoH / videoW;
        }
        else
        {
            canvasH = maxSize;
            canvasW = maxSize * videoW / videoH;
        }
        if (winW > winH)
        {
            canvasH *= float(winW) / float(winH);
            if (canvasH > maxSize)
            {
                canvasW *= maxSize / canvasH;
                canvasH = maxSize;
            }
        }
        else
        {
            canvasW *= float(winH) / float(winW);
            if (canvasW > maxSize)
            {
                canvasH *= maxSize / canvasW;
                canvasW = maxSize;
            }
        }
        mpVideoCanvas->setCorners(-canvasW, canvasH, canvasW, -canvasH);
    }
}