using System;
using System.Text;
using TMPro;
using UnityEngine;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;

public class MqttClientHandler : MonoBehaviour
{
    [SerializeField] string brokerIpAddress = "192.168.50.233"; // MQTT broker pc ip
    [SerializeField] int brokerPort = 1883;
    [SerializeField] string topic = "uwb/range";
    [SerializeField] TextMeshProUGUI mLogText;

    private MqttClient client;

    void Start()
    {
        client = new MqttClient(brokerIpAddress + ":" + brokerPort);
        client.MqttMsgPublishReceived += client_MqttMsgPublishReceived;
        string clientId = Guid.NewGuid().ToString();
        client.Connect(clientId);
        client.Subscribe(new string[] { topic }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_MOST_ONCE });

        LogAdd("Connected to MQTT broker at " + brokerIpAddress + ":" + brokerPort);
        LogAdd("Subscribed to topic: " + topic);
    }

    void client_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
    {
        string receivedMessage = Encoding.UTF8.GetString(e.Message);
        LogAdd("Received: " + receivedMessage, true);
        FindObjectOfType<DataHandler>().setData(receivedMessage);
    }

    void LogAdd(string t, bool isData = false)
    {
        if (isData)
        {
            mLogText.text = t;
        }
        else
        {
            mLogText.text += "\n" + t;
        }
    }
}

/*
M2Mqtt 라이브러리를 Unity에서 사용하려면 다음 단계를 따르시면 됩니다:

NuGet 패키지 매니저를 사용해서 M2Mqtt 패키지를 다운로드 받습니다. 이는 Windows 환경에서 .NET 프로젝트를 위한 패키지 매니저입니다.

설치:

NuGet는 Visual Studio에 포함되어 있습니다. Visual Studio가 설치된 경우, Visual Studio를 열고 새로운 프로젝트를 만들어 주세요 (프로젝트의 종류는 중요하지 않습니다).
프로젝트가 생성되면, 상단 메뉴에서 프로젝트 > NuGet 패키지 관리를 클릭합니다.
패키지 관리 탭에서 찾아보기를 클릭하고 M2Mqtt를 검색합니다.
패키지를 찾으면 설치를 클릭해서 패키지를 설치합니다.
패키지 설치 후, 패키지가 설치된 경로로 이동합니다. 일반적으로 이 경로는 프로젝트 폴더 내의 packages 디렉토리 안에 있습니다.

M2Mqtt.Net.4.3.0.0 폴더 (버전 번호는 다를 수 있음)를 찾아 lib\net45\M2Mqtt.Net.dll 파일을 찾습니다.

이 M2Mqtt.Net.dll 파일을 복사하여 Unity 프로젝트의 Assets 디렉토리에 붙여넣습니다.

Unity 에디터를 열고, 복사한 DLL 파일이 Assets 디렉토리에 있는지 확인합니다.

이제 Unity에서 using uPLibrary.Networking.M2Mqtt;를 통해 M2Mqtt 라이브러리를 사용할 수 있습니다.

또한, M2Mqtt는 .NET Framework 4.5를 필요로 합니다. 따라서 Unity에서 이를 사용하려면 Unity의 스크립팅 런타임 버전을 .NET 4.x 이상으로 설정해야 합니다. 이는 Unity 에디터의 Edit > Project Settings > Player > Other Settings > Configuration > Scripting Runtime Version에서 설정할 수 있습니다.
*/
