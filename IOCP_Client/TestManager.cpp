#include "TestManager.h"
#include <iostream>
#include <thread>
#include <algorithm>

TestManager::TestManager(const char* serverIP, int serverPort)
	: mServerIP(serverIP)
	, mServerPort(serverPort)
{
}

TestManager::~TestManager()
{
	DisconnectAllClients();
}

void TestManager::Connect(int numClients)
{
	mNumClients = numClients;

	CreateClients();
	ConnectAllClients();
	RegisterAllClients();
	LoginAllClients();
}

void TestManager::CreateClients()
{
	mClients.clear();

	for (int i = 0; i < mNumClients; i++)
	{
		string name = "TestUser" + to_string(i + 1);
		auto client = make_unique<TestClient>(i, name, mServerIP, mServerPort);
		mClients.push_back(move(client));
	}

	cout << "Created " << mNumClients << " test clients" << endl;
}

void TestManager::ConnectAllClients()
{
	cout << "Connecting all clients..." << endl;

	int successCount = 0;
	for (auto& client : mClients)
	{
		if (client->Connect())
			successCount++;
	}

	cout << "Connected: " << successCount << " / " << mClients.size() << endl;
}

void TestManager::LoginAllClients()
{
	cout << "Logging in all clients..." << endl;

	int loginSuccessCount = 0;

	for (int i = 0; i < (int)mClients.size(); ++i)
	{
		if (mClients[i]->Login(i))
		{
			++loginSuccessCount;
		}
	}

	cout << "Login complete: " << loginSuccessCount << " / " << mClients.size() << endl;
}

void TestManager::RegisterAllClients()
{
	cout << "Registering all clients..." << endl;

	int registerSuccessCount = 0;

	for (int i = 0; i < (int)mClients.size(); ++i)
	{
		if (mClients[i]->Register(i))
		{
			++registerSuccessCount;
		}
	}

	cout << "Register complete: " << registerSuccessCount << " / " << mClients.size() << endl;
}

void TestManager::DisconnectAllClients()
{
	for (auto& client : mClients)
	{
		client->Disconnect();
	}

	mClients.clear();
}

void TestManager::WaitForSeconds(int seconds)
{
	Sleep(seconds * 1000);
}

bool TestManager::WaitForLobbyChat(int expected, int timeoutSec)
{
	for (int i = 0; i < timeoutSec * 10; i++)
	{
		int totalReceived = 0;
		for (auto& client : mClients)
		{
			totalReceived += client->GetReceivedLobbyChatCount();
		}

		if (totalReceived >= expected)
			return true;

		if (i % 100 == 0 && i > 0)
		{
			cout << "  Progress: " << totalReceived << " / " << expected
				<< " (" << (totalReceived * 100 / expected) << "%)" << endl;
		}

		Sleep(100);
	}

	return false;
}

bool TestManager::WaitForWhisper(int expected, int timeoutSec)
{
	for (int i = 0; i < timeoutSec * 10; i++)
	{
		int totalReceived = 0;
		for (auto& client : mClients)
		{
			totalReceived += client->GetReceivedWhisperCount();
		}

		if (totalReceived >= expected)
			return true;

		Sleep(100);
	}

	return false;
}

bool TestManager::WaitForRoomChat(const vector<TestClient*>& roomClients, int expected, int timeoutSec)
{
	for (int i = 0; i < timeoutSec * 10; i++)
	{
		int totalReceived = 0;
		for (auto* client : roomClients)
		{
			totalReceived += client->GetReceivedRoomChatCount();
		}

		if (totalReceived >= expected)
			return true;

		Sleep(100);
	}

	return false;
}

void TestManager::SaveResultCSV(const string& filename, const string& header, const string& row)
{
	bool fileExists = false;
	{
		ifstream check(filename);
		fileExists = check.good();
	}

	ofstream file(filename, ios::app);
	if (!file.is_open())
	{
		cout << "Failed to open " << filename << endl;
		return;
	}

	if (!fileExists)
	{
		file << header << endl;
	}

	file << row << endl;
	file.close();

	cout << "Result saved to " << filename << endl;
}

// ============================================================
// Broadcast Test
// ============================================================
void TestManager::RunBroadcastTest(int messagesPerClient)
{
	cout << "\n========================================" << endl;
	cout << "BROADCAST TEST" << endl;
	cout << "Clients: " << mNumClients << ", Messages: " << messagesPerClient << endl;
	cout << "========================================\n" << endl;

	for (auto& client : mClients)
		client->ResetLobbyChatCount();

	WaitForSeconds(1);

	auto start = chrono::high_resolution_clock::now();

	for (int msg = 0; msg < messagesPerClient; msg++)
	{
		for (auto& client : mClients)
		{
			string message = "Broadcast message " + to_string(msg + 1);
			client->SendLobbyChat(message);
		}
		Sleep(50);
	}

	auto sendEnd = chrono::high_resolution_clock::now();
	auto sendMs = chrono::duration_cast<chrono::milliseconds>(sendEnd - start).count();
	cout << "All sends completed in " << sendMs << "ms. Waiting for responses..." << endl;

	int expected = (int)(mClients.size() * mClients.size() * messagesPerClient);
	bool allReceived = WaitForLobbyChat(expected, 60);

	auto end = chrono::high_resolution_clock::now();
	auto totalMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	int totalReceived = 0;
	int minCount = INT_MAX;
	int maxCount = 0;

	for (auto& client : mClients)
	{
		int count = client->GetReceivedLobbyChatCount();
		totalReceived += count;
		minCount = min(minCount, count);
		maxCount = max(maxCount, count);
	}

	int expectedPerClient = (int)(mClients.size() * messagesPerClient);

	cout << "\n=== BROADCAST STATISTICS ===" << endl;
	cout << "Total received: " << totalReceived << " / " << expected << endl;
	cout << "Per client - Min: " << minCount << ", Max: " << maxCount
		<< ", Expected: " << expectedPerClient << endl;
	cout << "Send time: " << sendMs << "ms" << endl;
	cout << "Total time: " << totalMs << "ms" << endl;
	cout << "Result: " << (allReceived ? "PASS" : "FAIL (timeout)") << endl;

	string csvHeader = "clients,messages,expected,received,send_ms,total_ms,result";
	string csvRow = to_string(mNumClients) + ","
		+ to_string(messagesPerClient) + ","
		+ to_string(expected) + ","
		+ to_string(totalReceived) + ","
		+ to_string(sendMs) + ","
		+ to_string(totalMs) + ","
		+ (allReceived ? "PASS" : "FAIL");
	SaveResultCSV("broadcast_results.csv", csvHeader, csvRow);

	cout << "========================================\n" << endl;
}

// ============================================================
// Whisper Test
// ============================================================
void TestManager::RunWhisperTest()
{
	cout << "\n========================================" << endl;
	cout << "WHISPER TEST" << endl;
	cout << "Clients: " << mNumClients << endl;
	cout << "========================================\n" << endl;

	for (auto& client : mClients)
		client->ResetWhisperCount();

	WaitForSeconds(1);

	auto start = chrono::high_resolution_clock::now();

	for (size_t i = 0; i < mClients.size(); i++)
	{
		size_t targetIdx = (i + 1) % mClients.size();
		string message = "Whisper from " + mClients[i]->GetName();
		mClients[i]->SendWhisper(mClients[targetIdx]->GetId(), message);
	}

	auto sendEnd = chrono::high_resolution_clock::now();
	auto sendMs = chrono::duration_cast<chrono::milliseconds>(sendEnd - start).count();
	cout << "All whispers sent in " << sendMs << "ms. Waiting for responses..." << endl;

	int expected = (int)mClients.size();
	bool allReceived = WaitForWhisper(expected, 30);

	auto end = chrono::high_resolution_clock::now();
	auto totalMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	int totalReceived = 0;
	for (auto& client : mClients)
		totalReceived += client->GetReceivedWhisperCount();

	cout << "\n=== WHISPER STATISTICS ===" << endl;
	cout << "Total received: " << totalReceived << " / " << expected << endl;
	cout << "Send time: " << sendMs << "ms" << endl;
	cout << "Total time: " << totalMs << "ms" << endl;
	cout << "Result: " << (allReceived ? "PASS" : "FAIL (timeout)") << endl;

	string csvHeader = "clients,expected,received,send_ms,total_ms,result";
	string csvRow = to_string(mNumClients) + ","
		+ to_string(expected) + ","
		+ to_string(totalReceived) + ","
		+ to_string(sendMs) + ","
		+ to_string(totalMs) + ","
		+ (allReceived ? "PASS" : "FAIL");
	SaveResultCSV("whisper_results.csv", csvHeader, csvRow);

	cout << "========================================\n" << endl;
}

// ============================================================
// Mixed Test
// ============================================================
void TestManager::RunMixedTest()
{
	cout << "\n========================================" << endl;
	cout << "MIXED TEST (Broadcast + Whisper)" << endl;
	cout << "Clients: " << mNumClients << endl;
	cout << "========================================\n" << endl;

	for (auto& client : mClients)
		client->ResetAllCounts();

	WaitForSeconds(1);

	int broadcastRounds = 5;
	int whisperRounds = 5;

	auto start = chrono::high_resolution_clock::now();

	for (int i = 0; i < broadcastRounds; i++)
	{
		mClients[0]->SendLobbyChat("Mixed broadcast " + to_string(i + 1));

		if (mClients.size() >= 2)
		{
			mClients[1]->SendWhisper(mClients[0]->GetId(), "Mixed whisper " + to_string(i + 1));
		}

		Sleep(50);
	}

	auto sendEnd = chrono::high_resolution_clock::now();
	auto sendMs = chrono::duration_cast<chrono::milliseconds>(sendEnd - start).count();

	int expectedBroadcast = broadcastRounds * (int)mClients.size();
	int expectedWhisper = (mClients.size() >= 2) ? whisperRounds : 0;

	WaitForLobbyChat(expectedBroadcast, 30);

	auto end = chrono::high_resolution_clock::now();
	auto totalMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	int totalBroadcast = 0;
	int totalWhisper = 0;
	for (auto& client : mClients)
	{
		totalBroadcast += client->GetReceivedLobbyChatCount();
		totalWhisper += client->GetReceivedWhisperCount();
	}

	cout << "\n=== MIXED STATISTICS ===" << endl;
	cout << "Broadcast received: " << totalBroadcast << " / " << expectedBroadcast << endl;
	cout << "Whisper received: " << totalWhisper << " / " << expectedWhisper << endl;
	cout << "Total time: " << totalMs << "ms" << endl;
	cout << "========================================\n" << endl;
}

// ============================================================
// Performance Test
// ============================================================
void TestManager::RunPerformanceTest()
{
	cout << "\n========================================" << endl;
	cout << "PERFORMANCE TEST" << endl;
	cout << "Clients: " << mNumClients << endl;
	cout << "========================================\n" << endl;

	int messageCounts[] = { 1, 5, 10, 50, 100 };

	for (int msgCount : messageCounts)
	{
		cout << "\n--- Testing with " << msgCount << " messages per client ---" << endl;

		RunBroadcastTest(msgCount);
		WaitForSeconds(2);
	}

	cout << "\n========================================" << endl;
	cout << "PERFORMANCE TEST COMPLETE" << endl;
	cout << "Results saved to broadcast_results.csv" << endl;
	cout << "========================================\n" << endl;
}

// ============================================================
// Room Test
// ============================================================
void TestManager::RoomTest()
{
	cout << "\n========================================" << endl;
	cout << "ROOM TEST" << endl;
	cout << "Clients: " << mNumClients << endl;
	cout << "========================================\n" << endl;

	if (mClients.size() < 2)
	{
		cout << "[SKIP] Need at least 2 clients for room test" << endl;
		return;
	}

	// Step 1: Client[0]이 방 생성
	cout << "[Step 1] Client[0] creates room 'GameRoom'..." << endl;
	bool createOk = mClients[0]->CreateRoom("GameRoom", 10);
	cout << "  CreateRoom: " << (createOk ? "OK" : "FAIL") << endl;

	if (!createOk)
	{
		cout << "Room test aborted: CreateRoom failed" << endl;
		return;
	}

	uint16_t roomId = mClients[0]->GetCurrentRoomId();
	cout << "  Room ID: " << roomId << endl;

	WaitForSeconds(1);

	// Step 2: 모든 클라이언트가 방 목록 조회
	cout << "\n[Step 2] All clients request room list..." << endl;
	int listOkCount = 0;
	for (auto& client : mClients)
	{
		if (client->RequestRoomList(0))
		{
			listOkCount++;
		}
	}
	cout << "  RoomList received: " << listOkCount << " / " << mClients.size() << endl;

	// 방 목록에서 생성된 방 확인 (성공한 클라이언트 중 아무거나 사용)
	bool roomFound = false;
	for (auto& client : mClients)
	{
		if (client->GetRoomListCount() == 0)
			continue;

		for (int i = 0; i < client->GetRoomListCount(); i++)
		{
			RoomInfo info = client->GetRoomListEntry(i);
			if (info.roomId == roomId)
			{
				roomFound = true;
				cout << "  Found room: id=" << info.roomId
					<< " name=" << info.roomName
					<< " users=" << info.curUserCount << "/" << info.maxUserCount << endl;
				break;
			}
		}
		if (roomFound) break;
	}
	cout << "  Room found in list: " << (roomFound ? "YES" : "NO") << endl;

	WaitForSeconds(1);

	// Step 3: Client[1..N-1] 방 입장
	cout << "\n[Step 3] Clients join the room..." << endl;
	int joinOkCount = 1; // Client[0]은 이미 방에 있음
	for (size_t i = 1; i < mClients.size(); i++)
	{
		bool joined = mClients[i]->JoinRoom(roomId);
		cout << "  " << mClients[i]->GetName() << " JoinRoom: " << (joined ? "OK" : "FAIL") << endl;
		if (joined)
			joinOkCount++;
		Sleep(100);
	}
	cout << "  Total in room: " << joinOkCount << " / " << mClients.size() << endl;

	WaitForSeconds(1);

	// Step 4: 방 안의 모든 클라이언트가 채팅 전송
	cout << "\n[Step 4] All clients send room chat..." << endl;
	const int msgsPerClient = 3;

	// 방에 있는 클라이언트 수집
	vector<TestClient*> inRoom;
	for (auto& client : mClients)
	{
		if (client->GetCurrentRoomId() == roomId)
		{
			client->ResetRoomChatCount();
			inRoom.push_back(client.get());
		}
	}

	for (int msg = 0; msg < msgsPerClient; msg++)
	{
		for (auto* client : inRoom)
		{
			client->SendRoomChat("Hello from " + client->GetName() + " #" + to_string(msg + 1));
		}
		Sleep(50);
	}

	// 기대값: 방 인원 수 × 메시지 수 × 방 인원 수 (모두가 모두의 메시지를 수신)
	int expectedRoomChat = (int)inRoom.size() * msgsPerClient * (int)inRoom.size();
	cout << "  Expected room chat notis: " << expectedRoomChat << endl;

	bool chatOk = WaitForRoomChat(inRoom, expectedRoomChat, 15);

	int totalRoomChatReceived = 0;
	for (auto* client : inRoom)
		totalRoomChatReceived += client->GetReceivedRoomChatCount();

	cout << "  Received: " << totalRoomChatReceived << " / " << expectedRoomChat << endl;
	cout << "  Room chat result: " << (chatOk ? "PASS" : "FAIL (timeout)") << endl;

	WaitForSeconds(1);

	// Step 5: 모든 클라이언트 방 퇴장
	cout << "\n[Step 5] All clients leave the room..." << endl;
	int leaveOkCount = 0;
	for (auto* client : inRoom)
	{
		bool left = client->LeaveRoom();
		cout << "  " << client->GetName() << " LeaveRoom: " << (left ? "OK" : "FAIL") << endl;
		if (left)
			leaveOkCount++;
		Sleep(100);
	}

	cout << "\n=== ROOM TEST SUMMARY ===" << endl;
	cout << "Create room:   " << (createOk ? "PASS" : "FAIL") << endl;
	cout << "Room list:     " << (roomFound ? "PASS" : "FAIL") << endl;
	cout << "Join room:     " << joinOkCount << "/" << mClients.size() << " joined" << endl;
	cout << "Room chat:     " << (chatOk ? "PASS" : "FAIL")
		<< " (" << totalRoomChatReceived << "/" << expectedRoomChat << ")" << endl;
	cout << "Leave room:    " << leaveOkCount << "/" << inRoom.size() << " left" << endl;
	cout << "========================================\n" << endl;
}
