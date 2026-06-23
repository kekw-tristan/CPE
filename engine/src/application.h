#pragma once

#include <memory>

namespace Engine
{
    namespace Logic
    {
        class cApplicationIntern;
    }

    struct sAppConfig
    {
        int width; 
        int height; 
        const char* _pTitle;
    };

    class cApplication
    {

        public:

            explicit cApplication(sAppConfig& _rAppConfig);
            virtual ~cApplication();

            cApplication(const cApplication&)               = delete; 
            cApplication& operator=(const cApplication&)    = delete; 

        public:

            void Run(); 

        protected:

            virtual void OnInit()                   {};
            virtual void OnShutdown()               {};          
            virtual void OnUpdate(float _deltaTime) {}; 
            virtual void OnDraw()                   {};            

        private:

            std::unique_ptr<Logic::cApplicationIntern> m_pAppIntern; 

    };
}