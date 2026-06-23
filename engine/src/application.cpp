#include "application.h"

#include "logic/applicationIntern.h"

#include <iostream>


namespace Engine
{
    cApplication::cApplication(sAppConfig& _rAppConfig)
        : m_pAppIntern(std::make_unique<Logic::cApplicationIntern>(_rAppConfig))
    {
    }


    cApplication::~cApplication()
    {
    }


    void cApplication::Run()
    {
        OnInit();

        while(!m_pAppIntern->GetShouldClose())
        {
            m_pAppIntern->Update();
            OnUpdate(m_pAppIntern->GetDeltaTime());

            m_pAppIntern->Draw();
            OnDraw();
        }
    }
}