"use client";
import "./styles.css";
import Image from "next/image";
import { useEffect, useState } from "react";

export default function Home() {
  const [logs, setLogs] = useState([{ date: "", time: "", data: "" }]);
  const [ip, setIp] = useState("");
  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await fetch(`http://${ip}`);
        if (!response.ok) {
          throw new Error("Network response was not ok");
        }
        const data = await response.json();
        setLogs(data);
      } catch (error) {
        console.error("Error fetching data:", error);
      }
    };
    if (ip == "") {
      const intervalId = setInterval(fetchData, 5000); // Fetch data every 5 seconds
      fetchData(); // Fetch data initially when component mounts
      return () => clearInterval(intervalId); // Cleanup function to clear interval
    }
    return;
  }, [ip]);

  const setIpValue = (e: any) => {
    e.preventDefult();
  };

  return (
    <main className="fixed top-10 left-10 right-10 p-24">
      <div className="panel h-full w-full">
        <form
          className="flex justify-between items-center"
          onSubmit={setIpValue}
        >
          <h1 className="text-6xl font-bold">Serial Monitor</h1>
          {/* <Image src="/serial-monitor.png" width={500} height={500} alt="logo" /> */}
          <p className="text-2xl">
            A simple web-based serial monitor to monitor serial data from your
            device.
          </p>
          <input
            type="text"
            name="deviceIP"
            className="px-5 rounded-lg py-2 "
            id="device-ip"
            placeholder="Enter Device IP"
            onChange={(e) => setIp(e.target.value)}
          />
        </form>

        <div className="monitor">
          {logs &&
            logs.map((log, i) => (
              <div className="row" key={i}>
                <div className="left-side">
                  <div className="col no">${i} </div>
                  <div className="col data">{log?.data}</div>
                </div>
                <div className="right-side">
                  <div className="date-time">
                    {log?.date}-{log?.time}
                  </div>
                </div>
              </div>
            ))}
          <div className="row" key={1}>
            <div className="left-side flex">
              <div className="col no">${1} </div>
              <div className="col data">avdsgrgsrg</div>
            </div>
            <div className="right-side">
              <div className="date-time">2024/04/2-10:00:00</div>
            </div>
          </div>
        </div>
      </div>
    </main>
  );
}
