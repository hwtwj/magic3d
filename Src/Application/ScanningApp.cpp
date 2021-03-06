#include "stdafx.h"
#include "ScanningApp.h"
#include "Tool/LogSystem.h"
#include "../Common/RenderSystem.h"
#include "../Common/ToolKit.h"
#include "../Common/AppManager.h"
//#include "../Common/MagicOgre.h"

namespace MagicApp
{
    ScanningApp::ScanningApp() : 
        mIsDeviceSetup(false)
    {
    }

    ScanningApp::~ScanningApp()
    {
    }

    bool ScanningApp::Enter(void)
    {
        InfoLog << "Enter ScanningApp" << std::endl;
        if (SetupDevice())
        {
            InfoLog << "Device Set Up Success." << std::endl;
            mIsDeviceSetup = true;
            mUI.Setup();
            SetupRenderScene();
            return true;
        }
        else
        {
            InfoLog << "Device Set Up Failed!" << std::endl;
            mIsDeviceSetup = false;
            MagicCore::AppManager::GetSingleton()->SwitchCurrentApp("Homepage");
            return false;
        }
    }

    bool ScanningApp::Update(float timeElapsed)
    {
        UpdateScannerDisplay();
        return true;
    }

    bool ScanningApp::Exit(void)
    {
        if (mIsDeviceSetup)
        {
            mUI.Shutdown();
            ReleaseDevice();
            ReleaseRenderScene();
        }
        return true;
    }

    bool ScanningApp::SetupDevice()
    {
        if (MagicCore::ToolKit::GetSingleton()->IsONIInitialized() == false)
        {
            openni::Status rc = openni::OpenNI::initialize();
            if (rc != openni::STATUS_OK)
            {
                InfoLog << "OpenNI initialize failed: " << openni::OpenNI::getExtendedError() << std::endl;
                return false;
            }
            else
            {
                InfoLog << "OpenNI initialize succeed" << std::endl;
                MagicCore::ToolKit::GetSingleton()->SetONIInitialized(true);
            }
        }

        openni::Status rc = mDevice.open(openni::ANY_DEVICE);
        if (rc != openni::STATUS_OK)
        {
            InfoLog << "Devive open failed: " << openni::OpenNI::getExtendedError() << std::endl;
            return false;
        }
        rc = mDepthStream.create(mDevice, openni::SENSOR_DEPTH);
        if (rc != openni::STATUS_OK)
        {
            InfoLog << "DepthStream create failed: " << openni::OpenNI::getExtendedError() << std::endl;
            return false;
        }
        rc = mColorStream.create(mDevice, openni::SENSOR_COLOR);
        if (rc != openni::STATUS_OK)
        {
            InfoLog << "ColorStream create failed: " << openni::OpenNI::getExtendedError() << std::endl;
            return false;
        }
        rc = mDepthStream.start();
        if (rc != openni::STATUS_OK)
        {
            InfoLog << "DepthStream start failed: " << openni::OpenNI::getExtendedError() << std::endl;
            return false;
        }
        rc = mColorStream.start();
        if (rc != openni::STATUS_OK)
        {
            InfoLog << "ColorStream start failed: " << openni::OpenNI::getExtendedError() << std::endl;
            return false;
        }

        return true;
    }

    void ScanningApp::SetupRenderScene()
    {
        MagicCore::RenderSystem::GetSingleton()->GetMainCamera()->setPosition(0, 0, 500);
        //MagicCore::RenderSystem::GetSingleton()->GetMainCamera()->setNearClipDistance(0.05);
        Ogre::SceneManager* pSceneMgr = MagicCore::RenderSystem::GetSingleton()->GetSceneManager();
        pSceneMgr->setAmbientLight(Ogre::ColourValue(0.1, 0.1, 0.1));
        Ogre::Light* frontLight = pSceneMgr->createLight("frontLight");
        frontLight->setPosition(0, 0, 500);
        frontLight->setDiffuseColour(0.8, 0.8, 0.8);
        frontLight->setSpecularColour(0.5, 0.5, 0.5);

    }

    void ScanningApp::ReleaseDevice()
    {
        InfoLog << "ScanningApp::ReleaseDevice" << std::endl;
        mColorStream.destroy();
        mDepthStream.destroy();
        mDevice.close();
    }

    void ScanningApp::ReleaseRenderScene()
    {
        InfoLog << "ScanningApp::ReleaseRenderScene" << std::endl;
        Ogre::SceneManager* pSceneMgr = MagicCore::RenderSystem::GetSingleton()->GetSceneManager();
        if (pSceneMgr->hasManualObject("ScannerDepth"))
        {
            pSceneMgr->getManualObject("ScannerDepth")->clear();
        }
        pSceneMgr->destroyLight("frontLight");
        MagicCore::RenderSystem::GetSingleton()->SetupCameraDefaultParameter();
    }

    void ScanningApp::StartRecord()
    {
        std::string fileName;
        char filterName[] = "ONI Files(*.oni)\0*.oni\0";
        MagicCore::ToolKit::FileSaveDlg(fileName, filterName);
        openni::Status rc  = mRecorder.create(fileName.c_str());
        if (rc != openni::STATUS_OK)
        {
            DebugLog << "Recorder create failed: " << openni::OpenNI::getExtendedError() << std::endl;
        }
        if (mRecorder.isValid())
        {
            rc = mRecorder.attach(mColorStream, true);
            if (rc != openni::STATUS_OK)
            {
                DebugLog << "ColorStream attach failed: " << openni::OpenNI::getExtendedError() << std::endl;
            }
            rc = mRecorder.attach(mDepthStream, false);
            if (rc != openni::STATUS_OK)
            {
                DebugLog << "DepthStream attach failed: " << openni::OpenNI::getExtendedError() << std::endl;
            }
            mRecorder.start();
        }
    }

    void ScanningApp::StopRecord()
    {
        mRecorder.stop();
        mRecorder.destroy();
    }

    void ScanningApp::UpdateScannerDisplay()
    {
        openni::VideoFrameRef depthFrame;
        int changeIndex;
        openni::VideoStream** pStreams = new openni::VideoStream*[1];
        pStreams[0] = &mDepthStream;
        openni::OpenNI::waitForAnyStream(pStreams, 1, &changeIndex);
        if (changeIndex == 0)
        {
            mDepthStream.readFrame(&depthFrame);
        }
        delete []pStreams;

        if (depthFrame.isValid())
        {
            const openni::DepthPixel* pDepth = (const openni::DepthPixel*)depthFrame.getData();
            int resolutionX = depthFrame.getVideoMode().getResolutionX();
            int resolutionY = depthFrame.getVideoMode().getResolutionY();
            std::vector<MagicMath::Vector3> posList;
            for(int y = 0; y < resolutionY; y++)  
            {  
                for(int x = 0; x < resolutionX; x++)  
                {
                    openni::DepthPixel depth = pDepth[y * resolutionX + x]; 
                    float rx, ry, rz;
                    openni::CoordinateConverter::convertDepthToWorld(mDepthStream, 
                        x, y, depth, &rx, &ry, &rz);
                   // MagicMath::Vector3 pos(-rx / 500.f, ry / 500.f, -rz / 500);
                    MagicMath::Vector3 pos(-rx, ry, -rz);
                    posList.push_back(pos);
                }
            }
            std::vector<MagicMath::Vector3> norList;
            for (int y = 0; y < resolutionY; y++)
            {
                for (int x = 0; x < resolutionX; x++)
                {
                    if ((y == 0) || (y == resolutionY - 1) || (x == 0) || (x == resolutionX - 1))
                    {
                        norList.push_back(MagicMath::Vector3(0, 0, 1));
                        continue;
                    }
                    if (posList.at(y * resolutionX + x)[2] > -(1.0e-15))
                    {
                        norList.push_back(MagicMath::Vector3(0, 0, 1));
                        continue;
                    }
                    MagicMath::Vector3 dirX = posList.at(y * resolutionX + x + 1) - posList.at(y * resolutionX + x - 1);
                    MagicMath::Vector3 dirY = posList.at((y + 1) * resolutionX + x) - posList.at((y - 1) * resolutionX + x);
                    MagicMath::Vector3 nor = dirX.CrossProduct(dirY);
                    double len = nor.Normalise();
                    if (len > 1.0e-15)
                    {
                        norList.push_back(nor);
                    }
                    else
                    {
                        norList.push_back(MagicMath::Vector3(0, 0, 1));
                    }
                }
            }
            //Rendering Point Set
            Ogre::ManualObject* pMObj = NULL;
            Ogre::SceneManager* pSceneMgr = MagicCore::RenderSystem::GetSingleton()->GetSceneManager();
            char psName[20] = "ScannerDepth";
            if (pSceneMgr->hasManualObject(psName))
            {
                pMObj = pSceneMgr->getManualObject(psName);
                pMObj->clear();
            }
            else
            {
                pMObj = pSceneMgr->createManualObject(psName);
                if (pSceneMgr->hasSceneNode("ModelNode"))
                {
                    pSceneMgr->getSceneNode("ModelNode")->attachObject(pMObj);
                }
                else
                {
                    pSceneMgr->getRootSceneNode()->createChildSceneNode("ModelNode")->attachObject(pMObj);
                }
            }
            pMObj->begin("MyCookTorrancePoint", Ogre::RenderOperation::OT_POINT_LIST);
            int pointNum = posList.size();
            for (int i = 0; i < pointNum; i++)
            {
                MagicMath::Vector3 pos = posList.at(i);
                if (pos[2] > -(1.0e-15))
                {
                    continue;
                }
                MagicMath::Vector3 nor = norList.at(i);
                pMObj->position(pos[0], pos[1], pos[2]);
                pMObj->normal(nor[0], nor[1], nor[2]);
                pMObj->colour(0.86, 0.86, 0.86);
            }
            pMObj->end();
        }
    }
}