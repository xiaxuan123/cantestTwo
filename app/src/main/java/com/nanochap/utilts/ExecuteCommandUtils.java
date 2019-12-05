package com.nanochap.utilts;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

/**
 * @author xiaoyi
 * @description 获取超级终端用户权限
 * @date 2019/12/5
 */
public class ExecuteCommandUtils {
    
    public static void initCommand(String command, boolean isRoot, boolean checkPermission) {
        Process process = null;
        DataOutputStream os = null;
        BufferedReader osReader = null;
        BufferedReader osErrorReader = null;

        try {
            //如果需要 root 权限则执行 su 命令，否则执行 sh 命令
            process = Runtime.getRuntime().exec(isRoot ? "su" : "sh");

            os = new DataOutputStream(process.getOutputStream());
            osReader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            osErrorReader = new BufferedReader(new InputStreamReader(process.getErrorStream()));

            //检查是否获取到 root 权限
            if (checkPermission && isRoot && !checkRootPermissionInProcess(os, process.getInputStream())) {
                return;
            }
            os.writeBytes(command + "\n");
            os.flush();
            System.out.println("command : " + command);

            os.writeBytes("exit\n");
            os.flush();

            String shellMessage;
            int processResult;
            String errorMessage;

            shellMessage = readOSMessage(osReader);
            errorMessage = readOSMessage(osErrorReader);
            processResult = process.waitFor();

            System.out.println("processResult : " + processResult);
            System.out.println("shellMessage : " + shellMessage);
            System.out.println("errorMessage : " + errorMessage);

        } catch (IOException | InterruptedException e) {
            e.printStackTrace();
        } finally {
            if (os != null) {
                try {
                    os.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            if (osReader != null) {
                try {
                    osReader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            if (osErrorReader != null) {
                try {
                    osErrorReader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            if (process != null) {
                process.destroy();
            }
        }
    }

    //读取执行命令后返回的信息
    private static String readOSMessage(BufferedReader messageReader) throws IOException {
        StringBuilder content = new StringBuilder();
        String lineString;
        while ((lineString = messageReader.readLine()) != null) {

            System.out.println("lineString : " + lineString);

            content.append(lineString).append("\n");
        }

        return content.toString();
    }

    //用 id 命令检查是否获取到 root 权限
    private static boolean checkRootPermissionInProcess(DataOutputStream os, InputStream osReader) throws IOException {
        String currentUid = readCommandResult(os, osReader, "id");
        System.out.println(currentUid);

        if (currentUid.contains("uid=0")) {
            System.out.println("ROOT: Root access granted");
            return true;
        } else {
            System.out.println("ROOT: Root access rejected");
            return false;
        }
    }

    //执行一个命令，并获得该命令的返回信息
    private static String readCommandResult(DataOutputStream os, InputStream in, String command) throws IOException {
        os.writeBytes(command + "\n");
        os.flush();

        return readCommandResult(in);
    }

    //读取命令返回信息
    private static String readCommandResult(InputStream in) throws IOException {
        StringBuilder result = new StringBuilder();

        //        System.out.println("Before : " + in.available());
        int available = 1;
        while (available > 0) {
            //            System.out.println("In : " + in.available());
            int b = in.read();
            result.append((char) b);
            //            System.out.println((char) b + " " + b);
            available = in.available();
        }
        //        System.out.println("After : " + in.available());

        return result.toString();
    }
}
