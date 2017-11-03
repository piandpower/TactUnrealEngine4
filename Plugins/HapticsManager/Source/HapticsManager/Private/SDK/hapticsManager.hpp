//Copyright bHaptics Inc. 2017

#ifndef BHAPTICS_HPP
#define BHAPTICS_HPP

#include "thirdparty/easywsclient.hpp"
#include "common/timer.hpp"
#include "common/model.hpp"
#include "common/util.hpp"
#include "Engine.h"

#ifdef _WIN32
#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>
#endif
#include <assert.h>

namespace bhaptics
{
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))  
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))  
    using namespace std;
    using easywsclient::WebSocket;
    class HapticPlayer
    {
		static HapticPlayer *hapticManager;
        unique_ptr<WebSocket> ws;
		vector<RegisterRequest> _registered;

		unique_ptr<PlayerRequest> _activeRequest;
		
		vector<string> _activeKeys;

        mutex mtx;
        int _currentTime = 0;
        int _interval = 20;
        int _motorSize = 20;
        HapticTimer timer;

		bool _enable = false;

        string host = "127.0.0.1";
        int port = 15881;
		string path = "v2/feedbacks";//old: "feedbacks";//

        int reconnectSec = 5;
        std::chrono::steady_clock::time_point prevReconnect;

		HapticPlayer() {}

        void reconnect()
        {
			if (!_enable)
			{
				return;
			}

            if (!retryConnection)
            {
                return;
            }

            if (connectionCheck())
            {
                return;
            }

            std::chrono::steady_clock::time_point current = std::chrono::steady_clock::now();

            int values = std::chrono::duration_cast<std::chrono::seconds>(current - prevReconnect).count();

            if (values > reconnectSec)
            {
                auto tried = unique_ptr<WebSocket>(WebSocket::create(host, port, path));
                if (tried)
                {
					tried->close();
					tried->poll();
                    ws.reset(WebSocket::create(host, port, path));
                }
	            ws.reset(WebSocket::create(host, port, path));
                prevReconnect = current;
            }
        }

		PlayerRequest* getActiveRequest()
		{
			if (!_activeRequest)
			{
				_activeRequest = unique_ptr<PlayerRequest>(PlayerRequest::Create());
			}
			return _activeRequest._Myptr();
		}

        bool connectionCheck()
        {
            if (!ws)
            {
                return false;
            }

            if (ws->getReadyState() == WebSocket::CLOSED)
            {
                ws.reset();
                return false;
            }

            return true;
        }

        void send()
        {
            if (!connectionCheck())
            {
                return;
            }

			TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
			getActiveRequest()->to_json(*JsonObject);

			FString OutputString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
			std::string jStr(TCHAR_TO_UTF8(*OutputString));
			mtx.lock();
            ws->send(jStr);
            ws->poll();
			_activeRequest = nullptr;
			mtx.unlock();
        }

        void updateActive(const string &key, const Frame& signal)
        {
			if (!_enable)
			{
				return;
			}
			SubmitRequest req;
			req.Frame = signal;
			req.Key = key;
			req.Type = "frame";
			getActiveRequest()->Submit.push_back(req);
			send();
        }

        void remove(const string &key)
        {
			if (!_enable)
			{
				return;
			}
			
			SubmitRequest req;
			req.Key = key;
			req.Type = "turnOff";
			getActiveRequest()->Submit.push_back(req);

			send();
        }

        void removeAll()
        {
			if (!_enable)
			{
				return;
			}

			SubmitRequest req;
			req.Type = "turnOffAll";

			getActiveRequest()->Submit.push_back(req);

			send();
        }

        void doRepeat()
        {
            reconnect();

            _currentTime += _interval;
        }

        void callbackFunc()
        {
            doRepeat();
        }


    public:
        bool retryConnection = true;

        int registerFeedback(const string &key, const string &filePath)
        {
            HapticFile file = Util::parse(filePath);
			RegisterRequest req;
			req.Key = key;
			req.Project = file.project;
			_registered.push_back(req);

			PlayerRequest* request = getActiveRequest();
			request->Register.push_back(req);

			send();
            return 0;
        }

        void init()
        {
			if (_enable)
				return;
            function<void()> callback = std::bind(&HapticPlayer::callbackFunc, this);
            timer.addTimerHandler(callback);
            timer.start();

#ifdef _WIN32
            INT rc;
            WSADATA wsaData;

            rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (rc) {
                printf("WSAStartup Failed.\n");
                return;
            }
#endif

            ws = unique_ptr<WebSocket>(WebSocket::create(host, port, path));

            connectionCheck();

			_enable = true;
        }

        void submit(const string &key, Position position, const vector<uint8_t> &motorBytes, int durationMillis)//change to submit - done
        {
			if (!_enable)
			{
				return;
			}

			vector<DotPoint> points;
			for (size_t i = 0; i < motorBytes.size(); i++)
			{
				if (motorBytes[i] > 0)
				{
					points.push_back(DotPoint(i,motorBytes[i]));
				}
			}

			Frame submitFrame = Frame::AsDotPointFrame(points, position, durationMillis);
			updateActive(key, submitFrame);
        }

		void submit(const string &key, Position position, const vector<DotPoint> &points, int durationMillis)
		{
			Frame req = Frame::AsDotPointFrame(points, position, durationMillis);
			updateActive(key, req);
		}

        void submit(const string &key, Position position, const vector<PathPoint> &points, int durationMillis)
        {
			Frame req = Frame::AsPathPointFrame(points, position, durationMillis);
            updateActive(key, req);
        }

        void submitRegistered(const string &key, float intensity, float duration)//change to submitRegistered
        {
            if (duration < 0.01f || duration > 100.0f)
            {
                printf("not allowed duration %f\n", duration);
                return;
            }

            if (intensity < 0.01f || intensity > 100.0f)
            {
                printf("not allowed intensity %f\n", duration);
                return;
            }

			SubmitRequest req;
			req.Key = key;
			req.Type = "key";
			req.Parameters["intensityRatio"] = intensity;
			req.Parameters["durationRatio"] = duration;

			getActiveRequest()->Submit.push_back(req);

			send();
        }

        void submitRegistered(const string &key)//change to submitRegistered - done
        {
			if (!_enable)
			{
				return;
			}
            
			SubmitRequest req;
			req.Key = key;
			req.Type = "key";

			getActiveRequest()->Submit.push_back(req);

			send();
        }

        bool isPlaying()
        {
		    return _activeKeys.size() > 0;
        }

        bool isPlaying(const string &key)
        {
			mtx.lock();
			bool ret = std::find(_activeKeys.begin(), _activeKeys.end(), key) != _activeKeys.end();
			mtx.unlock();
			return ret;
        }

        void turnOff()
        {
            removeAll();
        }

        void turnOff(const string &key)
        {
            remove(key);
        }

		void(*dispatchFunctionVar)(const char*);

		void checkMessage()
		{
			if (!ws)
			{
				return;
			}

			if (dispatchFunctionVar != NULL)
			{
				ws->dispatchChar(dispatchFunctionVar);
			}
			ws->poll();
		}

        void destroy()
        {
			_enable = false;
			
            if (!ws)
            {
                return;
            }

			ws->close();
			ws->poll();
			ws.reset();
        }

		void enableFeedback()
		{
			if (!ws)
			{
				return;
			}
			_enable = true;
		}

		void disableFeedback()
		{
			_enable = false;
		}

		void toggleFeedback()
		{
			if (!ws)
			{
				return;
			}

			_enable = !_enable;
		}

		map<string, vector<int>> parseResponse(FJsonObject j)
		{
			PlayerResponse response;
			response.from_json(j);
			_activeKeys = response.ActiveKeys;
			return response.Status;
		}

		HapticPlayer(HapticPlayer const&) = delete;
		void operator= (HapticPlayer const&) = delete;

		static HapticPlayer *instance()
		{
			if (!hapticManager)
			{
				hapticManager = new HapticPlayer();
			}

			return hapticManager;
		}
    };
}

#endif