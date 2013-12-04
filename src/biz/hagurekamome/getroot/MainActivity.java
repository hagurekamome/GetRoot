package biz.hagurekamome.getroot;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.StringWriter;

import android.os.Bundle;
import android.os.FileUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.app.Activity;

public class MainActivity extends Activity {
	private Button Button1;
	private Button Button2;
	private TextView textView1;
	private String cachePath;
	static {
		System.loadLibrary("getroot");
	}

	public native int native_getroot(String cachePath);
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		cachePath = this.getCacheDir().toString();
		setContentView(R.layout.activity_main);

		//Get Root Button
		Button1 = (Button)findViewById(R.id.button1);
		Button1.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				int result;

				File suFile = writeAssetToCacheFile("su", 00644);
				if(suFile == null){
					textView1.setText("su file not found");
					return;
				}

				File busyboxFile = writeAssetToCacheFile("busybox", 00755);
				if(busyboxFile == null){
					textView1.setText("busybox file not found");
					return;
				}

				File shFile = writeAssetToCacheFile("install_tool.sh", 00755);
				if(shFile == null){
					textView1.setText("install_tool.sh file not found");
					return;
				}

				File ricFile = writeAssetToCacheFile("00stop_ric", 00644);
				if(ricFile == null){
					textView1.setText("00stopric file not found");
					return;
				}

				File apkFile = writeAssetToCacheFile("Superuser.apk", 00644);
				if(apkFile == null){
					textView1.setText("SuperSU file not found");
					return;
				}
				try{
					result = native_getroot(cachePath);
				
					if(result == 0){
						Button1.setEnabled(false);
						textView1.setText("Get Root Success!!\n\n");
						textView1.append("Please Reboot.");
						Button2.setEnabled(true);
					}else{
						if (result == -1){
							textView1.setText("Get Root False!!\n\n");
							textView1.append("Not Support Device");
						}else if (result == -2){
							textView1.setText("Get Root False!!\n\n");
							textView1.append("Get Temproot false");
						}else {
							textView1.setText("Get TempRoot Success\n\n");
							textView1.append("But Shell Script Error\n\n");
							textView1.append("Error Code=" + Integer.toString(result));
							textView1.append("\nPlease Check /data/local/tmp/err.txt");
						}
					}

				}catch(Exception e){
					Log.e("getroot", e.toString());
					textView1.setText("Exception occured!!\n\n");
					textView1.append(e.toString());
				}

				suFile.delete();
				busyboxFile.delete();
				shFile.delete();
				ricFile.delete();
				apkFile.delete();
			}
		});
		
		//Reboot Buton
		Button2 = (Button)findViewById(R.id.button2);
		Button2.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				String result;
				result = executeScript("reboot.sh");
				textView1.append(result);
			}
		});
		Button2.setEnabled(false);

		textView1 = (TextView)findViewById(R.id.textView1);
	}

	private File writeAssetToCacheFile(String name, int mode) {
		return writeAssetToCacheFile(name, name, mode);
	}

	private File writeAssetToCacheFile(String assetName, String fileName, int mode) {
		return writeAssetToFile(assetName, new File(this.getCacheDir(), fileName), mode);
	}

	private File writeAssetToFile(String assetName, File targetFile, int mode) {
		try {
			InputStream in = this.getAssets().open(assetName);
			FileOutputStream out = new FileOutputStream(targetFile);
			byte[] buffer = new byte[1024];
			int len;
			while ((len = in.read(buffer)) > 0){
				out.write(buffer, 0, len);
			}
			in.close();
			out.close();
			FileUtils.setPermissions(targetFile.getAbsolutePath(), mode, -1, -1);

			return targetFile;
		} catch (IOException e) {
			e.printStackTrace();
			if (targetFile != null)
				targetFile.delete();

			return null;
		}
	}

	private String executeScript(String name) {
		File scriptFile = writeAssetToCacheFile(name, 00700);
		if (scriptFile == null)
			return "Could not find asset \"" + name + "\"";

		try {
			Process p = Runtime.getRuntime().exec(
					new String[] {
						"/system/xbin/su",
						"-c",
						scriptFile.getAbsolutePath() + " 2>&1"
					});
			BufferedReader stdout = new BufferedReader(new InputStreamReader(p.getInputStream()));
			BufferedReader stderr = new BufferedReader(new InputStreamReader(p.getErrorStream()));
			StringBuilder sb = new StringBuilder();
			String line;
			while ((line = stdout.readLine()) != null) {
				sb.append(line);
				sb.append('\n');
			}
			while ((line = stderr.readLine()) != null) {
				sb.append(line);
				sb.append('\n');
			}
			stdout.close();
			return sb.toString();

		} catch (IOException e) {
			StringWriter sw = new StringWriter();
			e.printStackTrace(new PrintWriter(sw));
			return sw.toString();
		} finally {
			scriptFile.delete();
		}
	}

}
